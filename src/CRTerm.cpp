// CRTerm.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <dwmapi.h>
#include <iostream>
#include <map>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "SDL_gpu.h"
#include "Shaders.h"
#include "Console.h"
#include "VT100.h"
#include "CRTermUI.h"
#include "CRTermConfig.h"
#include "ConfigEditor.h"
#include "ConfigSelector.h"
#include "ContextMenu.h"

#undef main

void menuCallBack(int item, void* term);

int main()
{
	// Read JSON file name from default file
	
	// Read configuration

	CRTermConfiguration* cfg = new CRTermConfiguration(GetDefaultConfigJSON());

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

	/* UI code */
	CRTermUIInstance* UI = new CRTermUIInstance(screen);

	ConfigEditor* cfg_edit = new ConfigEditor();
	cfg_edit->cfg = cfg;
	cfg_edit->show = false;

	ConfigSelector* cfg_load = new ConfigSelector();
	cfg_load->show = false;

	void** classes = (void**)calloc(3, sizeof(void*));

	classes[0] = (void*)vt100_term;
	classes[1] = (void*)cfg_edit;
	classes[2] = (void*)cfg_load;

	ContextMenu* cmenu = new ContextMenu(&menuCallBack, (void*)classes);

	cmenu->show = true;
	cmenu->Add("Copy");
	cmenu->Add("Paste");
	cmenu->Add("Select Config");
	cmenu->Add("Config Editor");

	UI->AddElement(cfg_edit);
	UI->AddElement(cfg_load);
	UI->AddElement(cmenu);

	bool toggle_context_menu = false;
	bool close_context_menu = false;
	while (!done)
	{
		while (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
			case SDL_QUIT:
				done = true;
				break;
			default:
				break;
			}
			UI->HandleEvent(ev);
			/* Don't handle events if config window is on */
			if (!cfg_edit->show && !cfg_load->show && !cmenu->menu_toggle)
			{
				/* Set the cursor to I beam */
				SDL_Cursor* cur = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
				SDL_SetCursor(cur);
				vt100_term->VT100HandleEvent(ev);
			}
		}
		GPU_Clear(screen);
		vt100_term->VT100Render(screen);
		UI->Render();
		GPU_Flip(screen);
	}
	SDL_Quit();
	exit(-1);
	return 0;

}

void menuCallBack(int item, void* data)
{
	void** classes = (void**)data;
	VT100* term_instance = (VT100*)classes[0];
	ConfigEditor* cfg_edit_instance = (ConfigEditor*)classes[1];
	ConfigSelector* cfg_selector_instance = (ConfigSelector*)classes[2];

	switch (item)
	{
	case 0:
		/* 0 = Copy */
		term_instance->VT100CopyToClipboard();
		break;
	case 1:
		/* Paste */
		term_instance->VT100PasteFromClipboard();
		break;
	case 2:
		cfg_selector_instance->show = true;
		break;
	case 3:
		/* Enable Config Editor */
		cfg_edit_instance->show = true;
		break;
	default:
		break;
	}
}