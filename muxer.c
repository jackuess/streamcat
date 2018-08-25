#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <libavformat/avformat.h>
#include <libavutil/error.h>

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

int mux(const char *out_filename, unsigned int n_in_files, char *in_files[n_in_files]) {
    int errnum;
    unsigned errline = 0;
    AVFormatContext *out_ctx = NULL;
    AVFormatContext *in_ctx[n_in_files];

    if ((errnum = avformat_alloc_output_context2(&out_ctx, NULL, NULL, out_filename)) < 0) {
        errline = __LINE__;
        goto finally;
    }
    if (out_ctx == NULL) {
        errline = __LINE__;
        goto finally;
    }

    for (unsigned int i = 0; i < n_in_files; i++) {
        in_ctx[i] = NULL;
        if ((errnum = avformat_open_input(&in_ctx[i], in_files[i], NULL, NULL)) < 0) {
            errline = __LINE__;
            goto finally;
        }
        if ((errnum = avformat_find_stream_info(in_ctx[i], NULL)) < 0) {
            errline = __LINE__;
            goto finally;
        }

        assert(in_ctx[i]->nb_streams == 1);
        av_dump_format(in_ctx[i], 0, in_files[i], 0);

        if ((errnum = add_stream(out_ctx, in_ctx[i]->streams[0])) < 0) {
            errline = __LINE__;
            goto finally;
        }
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

    for (unsigned int i = 0; i < n_in_files; i++) {
        if ((errnum = copy_stream(out_ctx, i, in_ctx[i], 0)) < 0) {
            errline = __LINE__;
        }
        avformat_close_input(&in_ctx[i]);
    }

    if ((errnum = av_write_trailer(out_ctx)) < 0) {
        errline = __LINE__;
    }

finally:
    if (out_ctx && !(out_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_ctx->pb);
    }
    avformat_free_context(out_ctx);

    if (errnum < 0 && errnum != AVERROR_EOF) {
        fprintf(stderr, "libavformat error: %s\n", av_err2str(errnum));
        return errline;
    }
    return 0;
}

int main(int argc, char *argv[argc+1]) {
    if (argc < 3) {
        printf("Usage: INPUT_FILE1 ... OUTPUT_FILE\n");
        return 1;
    }

    return mux(argv[argc-1], argc-2, &argv[1]);
}
