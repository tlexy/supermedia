#include <iostream>

#include "SDL2-2.30.3/include/SDL.h"

#undef main

int main()
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* window = SDL_CreateWindow("sdl_test", 300, 300, 450, 600, SDL_WINDOW_SHOWN);
	SDL_Surface* surface = SDL_GetWindowSurface(window);
	SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 255, 0, 0));
	SDL_UpdateWindowSurface(window);
	SDL_Delay(3000);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
}