#include <iostream>

#include "SDL2-2.30.3/include/SDL.h"

#undef main

int main()
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* window = SDL_CreateWindow("show bmp", 300, 300, 1280, 720, SDL_WINDOW_SHOWN);
	SDL_Surface* surface = SDL_GetWindowSurface(window);
	
	SDL_Surface* bmp_surface = SDL_LoadBMP("test1.bmp");

	SDL_BlitSurface(bmp_surface, NULL, surface, NULL);

	SDL_UpdateWindowSurface(window);
	SDL_Delay(3000);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
}