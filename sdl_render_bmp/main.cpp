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
	SDL_Texture* texture;
	renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_SetRenderDrawColor(renderer, 0x0, 0x0, 0x0, 0x0);
	SDL_RenderClear(renderer);

	SDL_Surface* surface = SDL_LoadBMP("test1.bmp");
	SDL_Rect box = { 0, 0, surface->w - 50, surface->h - 50 };

	texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_RenderCopy(renderer, texture, &box, &box);
	SDL_RenderPresent(renderer);

	SDL_Delay(3000);
	SDL_FreeSurface(surface);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
}