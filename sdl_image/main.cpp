#include <iostream>

#include "SDL2-2.30.3/include/SDL.h"
#include "SDL2_image-2.8.2/include/SDL_image.h"

#undef main

int main()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);

	SDL_Window* window = SDL_CreateWindow("show IMG", 300, 300, 1280, 720, SDL_WINDOW_SHOWN);
	
	SDL_Surface* surface = IMG_Load("bcd.PNG");
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(renderer);


	texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_RenderCopy(renderer, texture, &surface->clip_rect, &surface->clip_rect);
	SDL_RenderPresent(renderer);

	SDL_Event event;
	while (true) {
		SDL_PollEvent(&event);
		if (event.type == SDL_QUIT) {
			break;
		}
		SDL_Delay(1000);
	}

	SDL_Delay(3000);
	SDL_FreeSurface(surface);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
}