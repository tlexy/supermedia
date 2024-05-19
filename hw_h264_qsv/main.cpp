#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <queue>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixdesc.h>
#include <libavutil/hwcontext.h>
#include <libavutil/opt.h>
#include <libavutil/avassert.h>
#include <libavutil/imgutils.h>
}

#include "common/sdl/sdl_player.h"

SdlPlayer player("h264_qsv_decode");

static AVPixelFormat get_format(AVCodecContext* avctx, const enum AVPixelFormat* pix_fmts)
{
    while (*pix_fmts != AV_PIX_FMT_NONE) {
        if (*pix_fmts == AV_PIX_FMT_QSV) {
            return AV_PIX_FMT_QSV;
        }

        pix_fmts++;
    }

    fprintf(stderr, "The QSV pixel format not offered in get_format()\n");

    return AV_PIX_FMT_NONE;
}

static int decode_packet(AVCodecContext* decoder_ctx,
    AVFrame* frame, AVPacket* pkt)
{
    int ret = 0;

    ret = avcodec_send_packet(decoder_ctx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error during decoding\n");
        return ret;
    }

    while (ret >= 0) {
        int i, j;

        ret = avcodec_receive_frame(decoder_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            return ret;
        }

        AVFrame* sw_frame = av_frame_alloc();
        /* A real program would do something useful with the decoded frame here.
         * We just retrieve the raw data and write it to a file, which is rather
         * useless but pedagogic. */
        ret = av_hwframe_transfer_data(sw_frame, frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Error transferring the data to system memory\n");
            goto fail;
        }
        else {
            sw_frame->pts = frame->pkt_dts;
            player.push(sw_frame);
        }

    fail:

        if (ret < 0)
            return ret;
    }

    return 0;
}

//硬件解码测试qsv
int main()
{
    AVFormatContext* input_ctx = NULL;
    int video_index = -1;

    const char* filename = "../big2.mp4";
    int ret = avformat_open_input(&input_ctx, filename, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Cannot open input file '%s'\n", filename);
        exit(1);
    }

    //
    for (int i = 0; i < input_ctx->nb_streams; i++) {
        AVStream* st = input_ctx->streams[i];

        if (st->codecpar->codec_id == AV_CODEC_ID_H264)
            video_index = i;
    }
    if (video_index < 0) {
        fprintf(stderr, "No H.264 video stream in the input file\n");
        exit(1);
    }

    // open the hardware device
    AVBufferRef* device_ref = NULL;
    ret = av_hwdevice_ctx_create(&device_ref, AV_HWDEVICE_TYPE_QSV,
        "auto", NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Cannot open the hardware device\n");
        exit(1);
    }

    /* initialize the decoder */
    const AVCodec* decoder = avcodec_find_decoder_by_name("h264_qsv");
    if (!decoder) {
        fprintf(stderr, "The QSV decoder is not present in libavcodec\n");
        exit(1);
    }

    AVCodecContext* decoder_ctx = avcodec_alloc_context3(decoder);
    if (!decoder_ctx) {
        ret = AVERROR(ENOMEM);
        exit(1);
    }
    //
    decoder_ctx->codec_id = AV_CODEC_ID_H264;
    ///下面这是做什么？
    if (input_ctx->streams[video_index]->codecpar->extradata_size) {
        decoder_ctx->extradata = (uint8_t*)av_mallocz(input_ctx->streams[video_index]->codecpar->extradata_size +
            AV_INPUT_BUFFER_PADDING_SIZE);
        if (!decoder_ctx->extradata) {
            ret = AVERROR(ENOMEM);
            exit(1);
        }
        memcpy(decoder_ctx->extradata, input_ctx->streams[video_index]->codecpar->extradata,
            input_ctx->streams[video_index]->codecpar->extradata_size);

        decoder_ctx->extradata_size = input_ctx->streams[video_index]->codecpar->extradata_size;
    }

    ///这又是做什么？
    decoder_ctx->hw_device_ctx = av_buffer_ref(device_ref);
    decoder_ctx->get_format = get_format;

    ret = avcodec_open2(decoder_ctx, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error opening the decoder: ");
        exit(1);
    }

    player.init();
    player.start_play(decoder_ctx->width, decoder_ctx->height, input_ctx->streams[video_index]->time_base);

    AVFrame* frame = av_frame_alloc();
    AVPacket* pkt = av_packet_alloc();
    while (ret >= 0) {
        ret = av_read_frame(input_ctx, pkt);
        if (ret < 0)
            break;

        if (pkt->stream_index == video_index)
            ret = decode_packet(decoder_ctx, frame, pkt);

        av_packet_unref(pkt);
    }

    /* flush the decoder */
    ret = decode_packet(decoder_ctx, frame, NULL);

    player.destory();
    
	return 0;
}