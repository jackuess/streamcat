#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <libavformat/avformat.h>
#include <libavutil/error.h>

int copy_stream(AVFormatContext *dst, unsigned int dst_stream_index, AVFormatContext *src, unsigned int src_stream_index) {
    int errnum;
    AVPacket pkt;
    AVStream *outstream = dst->streams[dst_stream_index];
    AVStream *instream = src->streams[src_stream_index];

    while (1) {
        if ((errnum = av_read_frame(src, &pkt)) < 0) {
            return errnum;
        }

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

    return 0;
}

int main() {
    int errnum;
    unsigned errline = 0;
    AVFormatContext *videoctx = NULL;
    AVFormatContext *audioctx = NULL;
    AVFormatContext *outctx = NULL;
    AVStream *outstream = NULL;
    const char video_filename[] = "jacques/video.m4v";
    const char audio_filename[] = "jacques/audio.m4a";
    const char out_filename[] = "jacques/audiovideo.mp4";

    if ((errnum = avformat_open_input(&videoctx, video_filename, NULL, NULL)) < 0) {
        errline = __LINE__;
        goto finally;
    }
    if ((errnum = avformat_find_stream_info(videoctx, NULL)) < 0) {
        errline = __LINE__;
        goto finally;
    }
    if ((errnum = avformat_open_input(&audioctx, audio_filename, NULL, NULL)) < 0) {
        errline = __LINE__;
        goto finally;
    }
    if ((errnum = avformat_find_stream_info(audioctx, NULL)) < 0) {
        errline = __LINE__;
        goto finally;
    }

    assert(videoctx->nb_streams == 1);
    assert(audioctx->nb_streams == 1);

    av_dump_format(videoctx, 0, video_filename, 0);
    av_dump_format(audioctx, 0, audio_filename, 0);

    if ((errnum = avformat_alloc_output_context2(&outctx, NULL, NULL, out_filename)) < 0) {
        errline = __LINE__;
        goto finally;
    }
    if (outctx == NULL) {
        errline = __LINE__;
        goto finally;
    }

    if ((outstream = avformat_new_stream(outctx, NULL)) == NULL) {
        errline = __LINE__;
        goto finally;
    }
    if ((errnum = avcodec_parameters_copy(outstream->codecpar, videoctx->streams[0]->codecpar)) < 0) {
        errline = __LINE__;
        goto finally;
    }
    outstream->codecpar->codec_tag = 0;
    if ((outstream = avformat_new_stream(outctx, NULL)) == NULL) {
        errline = __LINE__;
        goto finally;
    }
    if ((errnum = avcodec_parameters_copy(outstream->codecpar, audioctx->streams[0]->codecpar)) < 0) {
        errline = __LINE__;
        goto finally;
    }
    outstream->codecpar->codec_tag = 0;

    av_dump_format(outctx, 0, out_filename, 1);

    if (!(outctx->oformat->flags & AVFMT_NOFILE)) {
        if ((errnum = avio_open(&outctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0) {
            errline = __LINE__;
            goto finally;
        }
    }

    if ((errnum = avformat_write_header(outctx, NULL)) < 0) {
        errline = __LINE__;
        goto finally;
    }

    if ((errnum = copy_stream(outctx, 0, videoctx, 0)) < 0) {
        errline = __LINE__;
    }
    if ((errnum = copy_stream(outctx, 1, audioctx, 0)) < 0) {
        errline = __LINE__;
    }

    if ((errnum = av_write_trailer(outctx)) < 0) {
        errline = __LINE__;
    }

    avformat_close_input(&videoctx);
    avformat_close_input(&audioctx);

finally:
    if (errnum < 0 && errnum != AVERROR_EOF) {
        char *buf = malloc(128);
        printf("Could not open file: %s\n", av_make_error_string(buf, 128, errnum));
        return errline;
    }
    return 0;
}
