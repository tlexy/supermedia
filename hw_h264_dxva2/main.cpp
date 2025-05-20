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
#include <libswscale/swscale.h>
}

#include "common/sdl/sdl_player.h"

SdlPlayer player("h264_dxva2_decode");

//static AVPixelFormat get_format(AVCodecContext* avctx, const enum AVPixelFormat* pix_fmts)
//{
//    while (*pix_fmts != AV_PIX_FMT_NONE) {
//        if (*pix_fmts == AV_PIX_FMT_CUDA) {
//            return AV_PIX_FMT_CUDA;// AV_PIX_FMT_QSV;
//        }
//
//        pix_fmts++;
//    }
//
//    fprintf(stderr, "The QSV pixel format not offered in get_format()\n");
//
//    return AV_PIX_FMT_NONE;
//}

struct SwsContext* g_sws_ctx = NULL;

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

        if (frame->format == AV_PIX_FMT_DXVA2_VLD) {
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
                if (!g_sws_ctx)
                {
                    g_sws_ctx = sws_getContext(
                        frame->width, frame->height, (AVPixelFormat)sw_frame->format,
                        sw_frame->width, sw_frame->height, AV_PIX_FMT_YUV420P,
                        SWS_BILINEAR, NULL, NULL, NULL);
                }
                AVFrame* yuv_frame = av_frame_alloc();
                if (av_image_alloc(yuv_frame->data, yuv_frame->linesize,
                    sw_frame->width, sw_frame->height, AV_PIX_FMT_YUV420P, 1) < 0) {
                    fprintf(stderr, "Could not allocate the output frame data\n");
                    av_frame_free(&yuv_frame);
                    goto fail;
                }
               /* av_image_alloc(rgb_frame->data, rgb_frame->linesize,
                    sw_frame->width, sw_frame->height, AV_PIX_FMT_BGR24, 1);*/
                sws_scale(g_sws_ctx, sw_frame->data, sw_frame->linesize,
                    0, sw_frame->height, yuv_frame->data, yuv_frame->linesize);


                yuv_frame->pts = frame->pkt_dts;//frame->pts;
                player.push(yuv_frame);
            }
        }

    fail:

        if (ret < 0)
            return ret;
    }

    return 0;
}

struct MyData {
    int x;
};

class CmpMyData {
public:
    bool operator()(const MyData& a, const MyData& b) {
        return  b.x < a.x;
    }
};

int main2() 
{
    std::priority_queue<MyData, std::vector<MyData>, CmpMyData> q1;
    MyData x, y, z;
    x.x = 1;
    y.x = 2;
    z.x = 3;
    q1.push(x);
    q1.push(y);
    q1.push(z);

    player.init();
    for (int i = 0; i < 10; ++i) {
        AVFrame* frame = av_frame_alloc();
        frame->pts = i;
        player.push(frame);
    }

    std::cin.get();
    return 0;
}

//硬件解码测试dxva2
int main()
{
    AVFormatContext* input_ctx = NULL;
    int video_index = -1;

    char errbuf[128];
    const char* filename = "../douna.mp4";
    int ret = avformat_open_input(&input_ctx, filename, NULL, NULL);
    if (ret < 0) {
        fprintf(stderr, "Cannot open input file '%s'\n", filename);
        exit(1);
    }
    if (avformat_find_stream_info(input_ctx, NULL) < 0) {
        fprintf(stderr, "Couldn't find stream information.\n");
        return -1;
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

    const AVCodec* codec = NULL;
    // 2. 查找视频流
    int video_stream_index = av_find_best_stream(input_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
    if (video_stream_index < 0) {
        fprintf(stderr, "Could not find video stream\n");
        exit(1);
    }

    // 3. 创建DXVA2硬件设备
    AVBufferRef* hw_device_ctx = NULL;
    AVHWDeviceType type = AV_HWDEVICE_TYPE_DXVA2;
    if (av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0) < 0) {
        fprintf(stderr, "Failed to create DXVA2 device\n");
        exit(1);
    }

    ///* initialize the decoder */
    //const AVCodec* decoder = avcodec_find_decoder_by_name("dxva2");
    //if (!decoder) {
    //    fprintf(stderr, "The dxva2 decoder is not present in libavcodec\n");
    //    exit(1);
    //}

    AVCodecContext* decoder_ctx = avcodec_alloc_context3(codec);
    if (!decoder_ctx) {
        ret = AVERROR(ENOMEM);
        exit(1);
    }
    ret = avcodec_parameters_to_context(decoder_ctx, input_ctx->streams[video_index]->codecpar);
    //
    //decoder_ctx->codec_id = AV_CODEC_ID_H264;
    decoder_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    decoder_ctx->get_format = [](AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) {
        const enum AVPixelFormat* p;
        for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
            if (*p == AV_PIX_FMT_DXVA2_VLD) {
                return *p;
            }
        }
        fprintf(stderr, "Failed to get DXVA2 pixel format\n");
        return AV_PIX_FMT_NONE;
    };

    //decoder_ctx->get_format = [](AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts) {
    //    const enum AVPixelFormat* p;
    //    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
    //        if (*p == AV_PIX_FMT_DXVA2_VLD) {
    //            return *p;  // 优先选择 DXVA2 格式
    //        }
    //    }
    //    // 如果 DXVA2 不可用，回退到软件解码格式（如 AV_PIX_FMT_YUV420P）
    //    for (p = pix_fmts; *p != AV_PIX_FMT_NONE; p++) {
    //        if (*p == AV_PIX_FMT_YUV420P || *p == AV_PIX_FMT_NV12) {
    //            return *p;
    //        }
    //    }
    //    fprintf(stderr, "No supported pixel format found\n");
    //    return AV_PIX_FMT_NONE;
    //};

    
    //hw_decoder_init(decoder_ctx, 
    ret = avcodec_open2(decoder_ctx, NULL, NULL);
    if (ret < 0) {
        memset(errbuf, 0x0, sizeof(errbuf));
        av_strerror(ret, errbuf, sizeof(errbuf));
        fprintf(stderr, "Error opening the decoder: %s", errbuf);
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

    player.over();
    std::cin.get();
    player.stop();
    player.wait();
    player.destory();
    
	return 0;
}