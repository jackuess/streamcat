#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <libavformat/avformat.h>
#include <libavutil/error.h>

struct SplitContext {
    AVFormatContext *audio_ctx;
    AVFormatContext *video_ctx;
};

int split_context_init(struct SplitContext *ctx, const char *audio_filename, const char *video_filename) {
    int errnum;

    if ((errnum = avformat_open_input(&ctx->audio_ctx, audio_filename, NULL, NULL)) < 0) {
        return errnum;
    }
    if ((errnum = avformat_find_stream_info(ctx->audio_ctx, NULL)) < 0) {
        return errnum;
    }
    if ((errnum = avformat_open_input(&ctx->video_ctx, video_filename, NULL, NULL)) < 0) {
        return errnum;
    }
    if ((errnum = avformat_find_stream_info(ctx->video_ctx, NULL)) < 0) {
        return errnum;
    }

    return 0;
}

int add_stream(AVFormatContext *dst_ctx, AVStream *src_stream) {
    int errnum = 0;
    AVStream *dst_stream = NULL;

    if ((dst_stream = avformat_new_stream(dst_ctx, NULL)) == NULL) {
        return 1;
    }
    if ((errnum = avcodec_parameters_copy(dst_stream->codecpar, src_stream->codecpar)) < 0) {
        return errnum;
    }
    dst_stream->codecpar->codec_tag = 0;

    return 0;
}

int copy_stream(AVFormatContext *dst, unsigned int dst_stream_index, AVFormatContext *src, unsigned int src_stream_index) {
    int errnum = 0;
    AVPacket pkt;
    AVStream *outstream = dst->streams[dst_stream_index];
    AVStream *instream = src->streams[src_stream_index];

    while ((errnum = av_read_frame(src, &pkt)) >= 0) {
        pkt.stream_index = dst_stream_index;
        pkt.pts = av_rescale_q_rnd(pkt.pts, instream->time_base, outstream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.dts = av_rescale_q_rnd(pkt.dts, instream->time_base, outstream->time_base, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
        pkt.duration = av_rescale_q(pkt.duration, instream->time_base, outstream->time_base);
        pkt.pos = -1;

        if ((errnum = av_interleaved_write_frame(dst, &pkt)) < 0) {
            return errnum;
        }
        av_packet_unref(&pkt);
    }

    return errnum;
}

int main(int argc, char *argv[argc+1]) {
    int errnum;
    unsigned errline = 0;
    struct SplitContext input_ctx = {NULL, NULL};
    AVFormatContext *out_ctx = NULL;

    if (argc != 4) {
        printf("Usage: AUDIO_FILE VIDEO_FILE OUTPUT_FILE\n");
        return 1;
    }

    const char *audio_filename = argv[1];
    const char *video_filename = argv[2];
    const char *out_filename = argv[3];

    if ((errnum = split_context_init(&input_ctx, audio_filename, video_filename)) < 0) {
        errline = __LINE__;
        goto finally;
    }
    assert(input_ctx.video_ctx->nb_streams == 1);
    assert(input_ctx.audio_ctx->nb_streams == 1);

    av_dump_format(input_ctx.audio_ctx, 0, audio_filename, 0);
    av_dump_format(input_ctx.video_ctx, 0, video_filename, 0);

    if ((errnum = avformat_alloc_output_context2(&out_ctx, NULL, NULL, out_filename)) < 0) {
        errline = __LINE__;
        goto finally;
    }
    if (out_ctx == NULL) {
        errline = __LINE__;
        goto finally;
    }

    if ((errnum = add_stream(out_ctx, input_ctx.audio_ctx->streams[0])) < 0) {
        errline = __LINE__;
        goto finally;
    }
    if ((errnum = add_stream(out_ctx, input_ctx.video_ctx->streams[0])) < 0) {
        errline = __LINE__;
        goto finally;
    }

    av_dump_format(out_ctx, 0, out_filename, 1);

    if (!(out_ctx->oformat->flags & AVFMT_NOFILE)) {
        if ((errnum = avio_open(&out_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0) {
            errline = __LINE__;
            goto finally;
        }
    }

    if ((errnum = avformat_write_header(out_ctx, NULL)) < 0) {
        errline = __LINE__;
        goto finally;
    }

    if ((errnum = copy_stream(out_ctx, 0, input_ctx.audio_ctx, 0)) < 0) {
        errline = __LINE__;
    }
    if ((errnum = copy_stream(out_ctx, 1, input_ctx.video_ctx, 0)) < 0) {
        errline = __LINE__;
    }

    if ((errnum = av_write_trailer(out_ctx)) < 0) {
        errline = __LINE__;
    }

    avformat_close_input(&input_ctx.video_ctx);
    avformat_close_input(&input_ctx.audio_ctx);
    if (out_ctx && !(out_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_ctx->pb);
    }
    avformat_free_context(out_ctx);

finally:
    if (errnum < 0 && errnum != AVERROR_EOF) {
        char *buf = malloc(128);
        printf("libavformat error: %s\n", av_make_error_string(buf, 128, errnum));
        return errline;
    }
    return 0;
}
