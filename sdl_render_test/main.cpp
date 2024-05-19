#include <iostream>

#include "SDL2-2.30.3/include/SDL.h"

#undef main

int main()
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* window = SDL_CreateWindow("render test", 300, 300, 1280, 720, SDL_WINDOW_SHOWN);
	
	SDL_Event event;
	SDL_Rect r;
	r.w = 100;
	r.h = 100;
	SDL_Renderer* renderer;
	//SDL_Texture* texture;
	renderer = SDL_CreateRenderer(window, -1, 0);
	//texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC, 1024, 768);//SDL_TEXTUREACCESS_STREAMING

	while (1) {
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT) {
			break;
		}
		else {
			std::cout << "event: " << event.type << std::endl;
		}
		r.x = rand() % 500;
		r.y = rand() % 500;

		//SDL_SetRenderTarget(renderer, texture);
		SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
		SDL_RenderClear(renderer);//用上面设置的颜色清空renderer
		//SDL_RenderDrawRect(renderer, &r);
		SDL_SetRenderDrawColor(renderer, 0x0, 0xFF, 0xbb, 0x0);
		SDL_RenderFillRect(renderer, &r);//用上面设置的颜色填充rect
		//SDL_SetRenderTarget(renderer, NULL);
		//SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
		SDL_Delay(1000);
	}

	SDL_Delay(3000);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
}