#include <SDL3/SDL.h>

#include <iostream>

int main()
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
	{
		std::cerr << "Failed to init SDL\n";
		return 1;
	}

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
	if (!SDL_CreateWindowAndRenderer("soundtoy", 1440, 810, flags, &window, &renderer))
	{
		std::cerr << "Failed to create window and renderer\n";
		return 1;
	}


	bool quit = false;
	SDL_Event event;
	while (!quit)
	{
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_EVENT_QUIT:
					quit = true;
					break;
				case SDL_EVENT_KEY_DOWN:
				{
					SDL_KeyboardEvent key = event.key;
					switch (key.scancode)
					{
						case SDL_SCANCODE_ESCAPE:
							quit = true;
							break;
						default: break;
					}

					break;
				}
				default: break;
			}
		}
	}

	SDL_Quit();

	return 0;
}
