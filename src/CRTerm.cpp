// CRTerm.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <map>
#include <Windows.h>
#include "SDL_gpu.h"
#include "Shaders.h"
#include "Console.h"
#include "VT100.h"
#include "CRTermConfig.h"

#undef main

int main()
{
	// Read configuration
	CRTermConfiguration* cfg = new CRTermConfiguration("default.json");

	// Set the screen resolution
	int resolution_x = (int)(cfg->font_width * cfg->font_scale * cfg->console_width);
	int resolution_y = (int)(cfg->font_height * cfg->font_scale * cfg->console_height);

	// Create the screen
	GPU_SetDebugLevel(GPU_DEBUG_LEVEL_MAX);
	GPU_Target* screen = GPU_Init(resolution_x, resolution_y, GPU_DEFAULT_INIT_FLAGS);
	if (screen == NULL)
	{
		return 1;
	}

	
	SDL_Event ev;
	bool done = false;
	VT100* vt100_term = new VT100(cfg);
	while (!done)
	{
		while (SDL_PollEvent(&ev))
		{
			if (ev.type == SDL_QUIT)
			{
				done = true;
			}

			vt100_term->VT100HandleEvent(ev);
		}
		GPU_ClearColor(screen, SDL_Color{ 0, 255, 0, 255 });
		vt100_term->con->Render(screen, 0, 0, cfg->font_scale);
		GPU_Flip(screen);
	}
	SDL_Quit();
	exit(-1);
	return 0;

}