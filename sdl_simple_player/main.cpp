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
	const char* filename = "../douna.mp4";
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

	SDL_Window* window = SDL_CreateWindow("Media Player",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!window) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window by SDL");
		exit(1);
	}
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Renderer by SDL");
		exit(1);
	}
	//AVPixelFormat
	int pixformat = SDL_PIXELFORMAT_IYUV;
	//AVPixelFormat pFormatCtx->streams[videoStream]->codecpar->format
	SDL_Texture* texture = SDL_CreateTexture(renderer,
		pixformat,
		SDL_TEXTUREACCESS_STREAMING,
		width,
		height);
	//¿ªÊ¼½âÂë¡£¡£¡£
	AVPacket packet;
	SDL_Rect rect;
	while (av_read_frame(pFormatCtx, &packet) >= 0) {
		if (packet.stream_index == videoStream) {
			ret = avcodec_send_packet(pCodecCtxOrig, &packet);
			if (ret < 0) {
				fprintf(stderr, "Error sending the frame to the encoder\n");
				return 0;
			}
			while (ret >= 0) {
				ret = avcodec_receive_frame(pCodecCtxOrig, pFrame);
				if (ret == AVERROR_EOF)
				{
					return 0;
				}
				else if (ret == AVERROR(EAGAIN))
				{
					break;
				}
				else if (ret < 0)
				{
					fprintf(stderr, "Error encoding audio frame\n");
					return 0;
				}

				// Did we get a video frame?
				if (ret >= 0) {
					SDL_UpdateYUVTexture(texture, NULL,
						pFrame->data[0], pFrame->linesize[0],
						pFrame->data[1], pFrame->linesize[1],
						pFrame->data[2], pFrame->linesize[2]);

					// Set Size of Window
					rect.x = 0;
					rect.y = 0;
					rect.w = pCodecCtxOrig->width;
					rect.h = pCodecCtxOrig->height;

					SDL_RenderClear(renderer);
					SDL_RenderCopy(renderer, texture, NULL, &rect);
					SDL_RenderPresent(renderer);
					SDL_Delay(41);
				}
			}
		}
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
}