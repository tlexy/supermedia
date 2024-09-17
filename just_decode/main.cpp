#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#undef main

int main()
{
	AVFormatContext* pFormatCtx = NULL;
	const char* filename = "../douna.mp4";
	if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
		exit(1);
	}

	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		exit(1);
	}

	av_dump_format(pFormatCtx, 0, filename, 0);

	int videoStream = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}

	if (videoStream == -1) {
		exit(1);
	}

	AVCodecContext* pCodecCtxOrig = avcodec_alloc_context3(NULL);
	int ret = avcodec_parameters_to_context(pCodecCtxOrig, pFormatCtx->streams[videoStream]->codecpar);
	if (ret < 0)
	{
		printf("find context failed from codecpar...");
		return 0;
	}

	//VCodec* pCodec = avcodec_find_decoder(pFormatCtx->streams[videoStream]->codecpar->codec_id);
	const AVCodec* pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
	if (pCodec == NULL) {
		exit(1);
	}
	if (avcodec_open2(pCodecCtxOrig, pCodec, NULL) < 0) {
		exit(1);
	}

	// Allocate video frame
	AVFrame* pFrame = av_frame_alloc();

	int width = pCodecCtxOrig->width;
	int height = pCodecCtxOrig->height;
	
	//ffmpeg # 判断AVPacket是否为关键帧
	//开始解码。。。
	AVPacket packet;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		if (packet.stream_index == videoStream) {
			if (packet.flags & AV_PKT_FLAG_KEY) {
				std::cout << "Key frame..." << std::endl;
			}
			else {
				std::cout << packet.pts << "\t" << packet.dts << "\t" << packet.duration << "\t" << packet.time_base.num << "/" << packet.time_base.den << std::endl;
			}
		}
	}
	return 0;
}