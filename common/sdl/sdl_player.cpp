#include "sdl/sdl_player.h"
#include "SDL2-2.30.3/include/SDL.h"

int SdlPlayer::_init = 0;
std::mutex SdlPlayer::_init_mux;

static int64_t now() {
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
	return now_ms.time_since_epoch().count();
}

SdlPlayer::SdlPlayer(const std::string& title)
	:_title(title)
{
}

SdlPlayer::~SdlPlayer()
{
}

void SdlPlayer::init()
{
	std::lock_guard<std::mutex> lock(_init_mux);
	if (_init == 0)
	{
		SDL_Init(SDL_INIT_EVERYTHING);
		++_init;
	}
}

void SdlPlayer::destory()
{
	std::lock_guard<std::mutex> lock(_init_mux);
	if (_init > 0)
	{
		SDL_Quit();
	}
}

AVFrame* SdlPlayer::get_frame_from_queue()
{
	std::lock_guard<std::mutex> lock(_que_mux);
	if (_que.size() >= 30 || _is_over)
	{
		if (!_que.empty())
		{
			auto item = _que.top();
			_que.pop();
			return item;
		}
	}
	return nullptr;
}

void SdlPlayer::start_play(int width, int height, AVRational time_base)
{
	init();

	_width = width;
	_height = height;
	_time_base = time_base;

	_th = std::make_shared<std::thread>(&SdlPlayer::_do_play, this);
}

void SdlPlayer::stop()
{
	_is_stop = true;
}

void SdlPlayer::_do_play()
{
	SDL_Window* window = SDL_CreateWindow(_title.c_str(),
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		_width, _height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	if (!window) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create window by SDL");
		return;
	}
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to create Renderer by SDL");
		return;
	}

	int pixformat = SDL_PIXELFORMAT_IYUV;
	SDL_Texture* texture = SDL_CreateTexture(renderer,
		pixformat,
		SDL_TEXTUREACCESS_STREAMING,
		_width,
		_height);

	SDL_Rect rect;
	// Set Size of Window
	rect.x = 0;
	rect.y = 0;
	rect.w = _width;
	rect.h = _height;
	int64_t last = 0;
	int64_t last_pts = 0;
	while (1)
	{
		if (_is_stop)
		{
			break;
		}
		AVFrame* frame = get_frame_from_queue();
		if (!frame)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		SDL_UpdateYUVTexture(texture, NULL,
			frame->data[0], frame->linesize[0],
			frame->data[1], frame->linesize[1],
			frame->data[2], frame->linesize[2]);

		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "frame pts: %lld", frame->pts);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, texture, NULL, &rect);
		if (last != 0)
		{
			//自动计算要睡眠的时间
			int64_t diff = frame->pts - last_pts;
			int64_t interval = (diff * 1000 / _time_base.den);
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

void SdlPlayer::over()
{
	_is_over = true;
}

void SdlPlayer::push(const AVFrame* frame)
{
	std::lock_guard<std::mutex> lock(_que_mux);
	
	_que.push(const_cast<AVFrame*>(frame));
}