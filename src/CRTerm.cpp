/* The entry point of our glorious terminal! */
#include <Windows.h>
#include <dwmapi.h>
#include <iostream>
#include <map>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "SDL_gpu.h"
#include "CustomTitleBar.h"
#include "Shaders.h"
#include "Console.h"
#include "VT100.h"
#include "CRTermUI.h"
#include "CRTermConfig.h"
#include "ConfigEditor.h"
#include "ConfigSelector.h"
#include "ContextMenu.h"


/* SDLmain requires this. It seems to define its own main. */
#undef main

void menuCallBack(int, void*);

int main()
{
	/* Read the path of the configuration JSON from "default" and then load it */
	CRTermConfiguration* cfg = new CRTermConfiguration(GetDefaultConfigJSON());

	/* Calculate the required screen resolution from the configuration */
	int resolution_x = (int)(cfg->font_width * cfg->font_scale * cfg->console_width) + 2 * SIDES_WIDTH;
	int resolution_y = (int)(cfg->font_height * cfg->font_scale * cfg->console_height) + TITLE_BAR_HEIGHT + SIDES_WIDTH;

	/* Create the screen */
	GPU_SetDebugLevel(GPU_DEBUG_LEVEL_MAX);
	GPU_Target* screen = GPU_Init(resolution_x, resolution_y, GPU_DEFAULT_INIT_FLAGS);

	if (screen == NULL)
	{
		return 1;
	}

	SDL_SetWindowTitle(SDL_GetWindowFromID(screen->context->windowID), "CRTerm.exe starting...");

	SDL_Event ev;
	bool done = false;
	
	VT100* vt100_term = new VT100(cfg, screen);
	/* Add the offsety as the custom titlebar height */
	vt100_term->screen_offsety = TITLE_BAR_HEIGHT;
	vt100_term->screen_offsetx = SIDES_WIDTH;
	/* UI code */
	CRTermUIInstance* UI = new CRTermUIInstance(screen);

	/* Prepare custom title bar */
	CustomTitleBar* title = new CustomTitleBar(screen);
	title->show = true;

	ConfigEditor* cfg_edit = new ConfigEditor();
	cfg_edit->cfg = cfg;
	cfg_edit->show = false;

	ConfigSelector* cfg_load = new ConfigSelector();
	cfg_load->show = false;

	/* Store all the allocated classes in a nice array that'll be passed to the callback */
	void** tocallback = (void**)calloc(3, sizeof(void*));

	tocallback[0] = (void*)vt100_term;
	tocallback[1] = (void*)cfg_edit;
	tocallback[2] = (void*)cfg_load;

	ContextMenu* cmenu = new ContextMenu(&menuCallBack, (void*)tocallback);
	cmenu->show = true;
	cmenu->Add("Copy");
	cmenu->Add("Paste");
	cmenu->Add("Select Config");
	cmenu->Add("Config Editor");
	cmenu->Add("Send ^C");

	UI->AddElement(title);
	UI->AddElement(cfg_edit);
	UI->AddElement(cfg_load);
	UI->AddElement(cmenu);

	SDL_Cursor* ibeam_cur = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	SDL_Cursor* normal_cur = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	
	int mouse_x = 0, mouse_y = 0;
	SDL_GetMouseState(&mouse_x, &mouse_y);
	while (!done)
	{
		while (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
			case SDL_QUIT:
				done = true;
				break;
			case SDL_MOUSEMOTION:
				SDL_GetMouseState(&mouse_x, &mouse_y);
				break;
			default:
				break;
			}
			UI->HandleEvent(ev);
			/* Don't handle events if config window is on */
			if (!cfg_edit->show && !cfg_load->show && !cmenu->menu_toggle && mouse_y > TITLE_BAR_HEIGHT)
			{
				/* Set the cursor to I beam if normal operation of terminal, if not in titlebar */
				SDL_SetCursor(ibeam_cur);
				vt100_term->VT100HandleEvent(ev);
			}
			else
			{
				/* Set the cursor to normal if a UI element is online */
				SDL_SetCursor(normal_cur);
			}
		}
		GPU_ClearColor(screen, SDL_Color{ 40, 40, 40, 255 });
		/* First render the terminal */
		vt100_term->VT100Render();
		/* Then the UI */
		UI->Render();
		GPU_Flip(screen);
	}
	SDL_Quit();
	return 0;
}

void menuCallBack(int item, void* data)
{
	void** classes = (void**)data;
	VT100* term_instance = (VT100*)classes[0];
	ConfigEditor* cfg_edit_instance = (ConfigEditor*)classes[1];
	ConfigSelector* cfg_selector_instance = (ConfigSelector*)classes[2];

	/* ContextMenu preserves order in which the elements were added. */
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
		/* Enable config selector */
		cfg_selector_instance->show = true;
		break;
	case 3:
		/* Enable Config Editor */
		cfg_edit_instance->show = true;
		break;
	case 4:
		/* Send ^C */
		term_instance->VT100Send("\x3");
		break;
	default:
		break;
	}
}