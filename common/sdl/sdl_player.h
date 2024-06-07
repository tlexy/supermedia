#ifndef COMMON_SDL_SDL_PLAYER_H
#define COMMON_SDL_SDL_PLAYER_H

#include <chrono>
#include <thread>
#include <memory>
#include <queue>
#include <mutex>
#include <string>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

class Cmp {
public:
	bool operator()(const AVFrame* a, const AVFrame* b) {
		if (a->pts < 0 || b->pts < 0) {
			fprintf(stderr, "%lld, %lld", a->pts, b->pts);
		}
		return  b->pts < a->pts;
	}
};

class SdlPlayer
{
public:
	SdlPlayer(const std::string& title);
	~SdlPlayer();

	static void init();
	static void destory();
	void start_play(int width, int height, AVRational time_base);
	void push(const AVFrame*);
	void over();
	void stop();
	void wait();

private:
	void _do_play();
	AVFrame* get_frame_from_queue();

private:
	std::shared_ptr<std::thread> _th{nullptr};
	int _width;
	int _height;
	std::string _title;
	AVRational _time_base;
	bool _is_over{ false };
	std::priority_queue<AVFrame*, std::vector<AVFrame*>, Cmp> _que;
	std::mutex _que_mux;
	bool _is_stop{ false };

	static int _init;
	static std::mutex _init_mux;
};


#endif