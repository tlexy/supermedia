#include <iostream>
#include <chrono>
#include <thread>
#include <memory>
#include <queue>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}
#include "SDL2-2.30.3/include/SDL.h"

#undef main

class Cmp {
public:
	bool operator()(const AVFrame* a, const AVFrame* b) {
		return a->pts >= b->pts;
	}
};

std::mutex mux;
bool done = false;
std::priority_queue<AVFrame*, std::vector<AVFrame*>, Cmp> que;

int64_t now() {
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
	return now_ms.time_since_epoch().count();
}

void init() {
	SDL_Init(SDL_INIT_EVERYTHING);
}

AVFrame* get_frame_from_queue()
{
	std::lock_guard<std::mutex> lock(mux);
	if (que.size() >= 30 || done)
	{
		if (!que.empty())
		{
			auto item = que.top();
			que.pop();
			return item;
		}
	}
	return nullptr;
}

void player_func(int width, int height, AVRational time_base) {

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

	int pixformat = SDL_PIXELFORMAT_IYUV;
	SDL_Texture* texture = SDL_CreateTexture(renderer,
		pixformat,
		SDL_TEXTUREACCESS_STREAMING,
		width,
		height);

	SDL_Rect rect;
	int64_t last = 0;
	int64_t last_pts = 0;
	while (1)
	{
		AVFrame* frame = get_frame_from_queue();
		if (!frame)
		{
			continue;
		}

		SDL_UpdateYUVTexture(texture, NULL,
			frame->data[0], frame->linesize[0],
			frame->data[1], frame->linesize[1],
			frame->data[2], frame->linesize[2]);
		
		// Set Size of Window
		rect.x = 0;
		rect.y = 0;
		rect.w = width;
		rect.h = height;

		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "frame pts: %lld", frame->pts);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, &rect);
		if (last != 0)
		{
			//自动计算要睡眠的时间
			int64_t diff = frame->pts - last_pts;
			int64_t interval = (diff * 1000 / time_base.den);
			auto tt = now();
			auto interval2 = tt - last;//真实的时间间隔
			auto s_diff = interval2 - interval;
			if (s_diff < 0)
			{
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "sleep: %d", interval - interval2);
				std::this_thread::sleep_for(std::chrono::milliseconds(interval - interval2));
			}
		}
		last = now();
		SDL_RenderPresent(renderer);
		//last = now();
		last_pts = frame->pts;

		av_frame_free(&frame);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

//一个解码线程，一个播放线程。使用优先级队列，对解码出来的帧使用pts进行排序，然后再渲染
int main()
{
	init();
	
	AVFormatContext* pFormatCtx = NULL;
	const char* filename = "../test2.mp4";//douna.mp4
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
		AVStream* stream = pFormatCtx->streams[i];
		if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
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

	int width = pCodecCtxOrig->width;
	int height = pCodecCtxOrig->height;

	//播放线程
	auto th = std::make_shared<std::thread>(player_func, width, height, pFormatCtx->streams[videoStream]->time_base);
	
	//开始解码。。。
	bool to_do = true;
	AVPacket packet;
	while (true) {
		{
			std::lock_guard<std::mutex> lock(mux);
			if (que.size() <= 30) {
				to_do = true;
			}
		}
		if (!to_do) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}
		ret = av_read_frame(pFormatCtx, &packet);
		if (ret < 0) {
			break;
		}
		//avformat_find_stream_info
		if (packet.stream_index == videoStream) {
			ret = avcodec_send_packet(pCodecCtxOrig, &packet);
			if (ret < 0) {
				fprintf(stderr, "Error sending the frame to the encoder\n");
				return 0;
			}
			
			while (ret >= 0) {
				AVFrame* pFrame = av_frame_alloc();
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
					std::lock_guard<std::mutex> lock(mux);
					//pFrame->pts = packet.pts;
					pFrame->pts = pFrame->pkt_dts;
					que.push(pFrame);
					if (que.size() >= 60) {
						to_do = false;
					}
				}
			}
		}
	}
	done = true;
	//th->join();

	SDL_Quit();
	return 0;
}