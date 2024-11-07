#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include "SDL2-2.30.3/include/SDL.h"

#undef main

int main()
{
	SDL_Init(SDL_INIT_EVERYTHING);	
	
	AVFormatContext* pFormatCtx = NULL;
	const char* filename = "../testb.mp4";
	if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open video file!");
		exit(1);
	}

	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to find stream infomation!");
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
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Din't find a video stream!");
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
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unsupported codec!\n");
		exit(1);
	}
	if (avcodec_open2(pCodecCtxOrig, pCodec, NULL) < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to open decoder!\n");
		exit(1);
	}

	// Allocate video frame
	AVFrame* pFrame = av_frame_alloc();

	int width = pCodecCtxOrig->width;
	int height = pCodecCtxOrig->height;

	int image_frame_count = 0;
	int video_packet_count = 0;
	char strbuf[128];
	//AVPixelFormat
	int pixformat = SDL_PIXELFORMAT_IYUV;
	//AVPixelFormat pFormatCtx->streams[videoStream]->codecpar->format
	//开始解码。。。
	AVPacket packet;
	SDL_Rect rect;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		if (packet.stream_index == videoStream) {
			++video_packet_count;
			ret = avcodec_send_packet(pCodecCtxOrig, &packet);
			if (ret < 0) {
				fprintf(stderr, "Error sending the frame to the encoder\n");
				return 0;
			}
			while (ret >= 0) {
				ret = avcodec_receive_frame(pCodecCtxOrig, pFrame);
				if (ret == AVERROR_EOF)
				{
					fprintf(stderr, "AVERROR_EOF\n");
					break;
				}
				else if (ret == AVERROR(EAGAIN))
				{
					//fprintf(stderr, "AVERROR(EAGAIN)\n");
					/*memset(strbuf, 0x0, sizeof(strbuf));
					av_strerror(ret, strbuf, sizeof(strbuf));
					fprintf(stderr, "decode error: %d, msg: %s", ret, strbuf);
					*/
					break;
				}
				else if (ret < 0)
				{
					fprintf(stderr, "Error encoding audio frame\n");
					return 0;
				}

				// Did we get a video frame?
				if (ret >= 0) {
					
					++image_frame_count;
				}
				else {
					memset(strbuf, 0x0, sizeof(strbuf));
					av_strerror(ret, strbuf, sizeof(strbuf));
					fprintf(stderr, "decode error: %d, msg: %s", ret, strbuf);
				}
			}
		}
	}
	//刷出编码器中缓存的帧
	ret = avcodec_send_packet(pCodecCtxOrig, NULL);
	while (ret >= 0)
	{
		ret = avcodec_receive_frame(pCodecCtxOrig, pFrame);
		if (ret >= 0) 
		{
			++image_frame_count;
		}
	}

	fprintf(stderr, "frameCount: %d, %d", image_frame_count, video_packet_count);
	SDL_Quit();
	return 0;
}