/* The entry point of our glorious terminal! */

#include <Windows.h>
#include <dwmapi.h>
#include <iostream>
#include <map>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#define SDL_MAIN_HANDLED
#include "SDL_gpu.h"
#include "CustomTitleBar.h"
#include "Console.h"
#include "VT100.h"
#include "CRTermUI.h"
#include "CRTermConfig.h"
#include "ConfigEditor.h"
#include "ConfigSelector.h"
#include "ContextMenu.h"
#include "ArgumentParser.h"
#include "CRTerm.h"
#include "ConsoleFont.h"
#include "PNGFont.h"
#include "TrueType.h"
#include "Win32Transparency.h"

/* SDLmain requires this. It seems to define its own main. */
#undef main

void menuCallBack(int, void*);

int main(int argc, char* argv[])
{
	/* Read the path of the configuration JSON from "default" and then load it */
	CRTermConfiguration* cfg = new CRTermConfiguration(GetDefaultConfigJSON());

	/* Parse arguments */
	ArgumentParser arg_parse;
	arg_parse.AddArgument("fs");
	arg_parse.AddArgument("cw");
	arg_parse.AddArgument("ch");
	arg_parse.AddArgument("fscrn");
	arg_parse.AddArgument("cmd", true);
	arg_parse.Parse(argc, argv);

	arg_parse.GetArgument("cw", cfg->console_width);
	arg_parse.GetArgument("ch", cfg->console_height);
	arg_parse.GetArgument("fs", cfg->font_scale);
	arg_parse.GetArgument("cmd", cfg->shell_command);
	arg_parse.GetArgument("fscrn", cfg->fullscreen);

	/* Create the screen */
	GPU_SetDebugLevel(GPU_DEBUG_LEVEL_MAX);

	/* Default to 640x480, we'll calculate the required terminal resolution later. */
	GPU_Target* screen = GPU_Init(640, 480, GPU_DEFAULT_INIT_FLAGS);
	if (screen == NULL)
	{
		return 1;
	}

	/* Load up the font */
	ConsoleFont* fnt;
	/* Check if font if TTF or PNG */
	if (endsWith(cfg->bitmap_font_file, ".png"))
	{
		fnt = new PNGBitmapFont(cfg->bitmap_font_file, cfg->font_width, cfg->font_height);
	}
	else if (endsWith(cfg->bitmap_font_file, ".ttf"))
	{
		fnt = new FreeTypeFont(cfg->bitmap_font_file, TRUETYPE_DEFAULT_FONT_SIZE);
	}
	else
	{
		std::cerr << "Unsupported font type detected: " << cfg->bitmap_font_file << ". Exitting." << std::endl;
		return -1;
	}

	/* Calculate the required screen resolution from the font */
	int resolution_x, resolution_y;

	/* If full screen is chosen, then we must modify console width and height */
	if (cfg->fullscreen)
	{
		int sw = GetSystemMetrics(SM_CXSCREEN);
		int sh = GetSystemMetrics(SM_CYSCREEN);

		resolution_x = sw;
		resolution_y = sh;

		cfg->console_width = sw / (fnt->GetXAdvance() * cfg->font_scale);
		cfg->console_height = sh / (fnt->GetYAdvance() * cfg->font_scale);
	}
	else 
	{
		resolution_x = (int)(fnt->GetXAdvance() * cfg->font_scale * cfg->console_width) + 2 * SIDES_WIDTH;
		resolution_y = (int)(fnt->GetYAdvance() * cfg->font_scale * cfg->console_height) + TITLE_BAR_HEIGHT + SIDES_WIDTH;
	}
	
	cfg->resolution_x = resolution_x;
	cfg->resolution_y = resolution_y;
	GPU_SetWindowResolution(resolution_x, resolution_y);
	
	/* If full screen go full screen! */
	if(cfg->fullscreen)
		GPU_SetFullscreen(true, true);

	SDL_SetWindowTitle(SDL_GetWindowFromID(screen->context->windowID), "CRTerm.exe starting...");

	/*
		Make window transparent. 
	*/
	Win32SetWindowTransparency(cfg->opacity);

	SDL_Event ev;
	bool done = false;
	
	VT100* vt100_term = new VT100(cfg, fnt, screen);
	/* Add the offsety as the custom titlebar height */
	vt100_term->screen_offsety = TITLE_BAR_HEIGHT;
	vt100_term->screen_offsetx = SIDES_WIDTH;
	/* UI code */
	CRTermUIInstance* UI = new CRTermUIInstance(screen);

	/* Prepare custom title bar */
#ifdef CRTERM_CUSTOM_TITLE_BAR
	CustomTitleBar* title = new CustomTitleBar(screen);
	title->show = true;
#endif
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
	cmenu->Add("Exit");

#ifdef CRTERM_CUSTOM_TITLE_BAR
	UI->AddElement(title);
#endif
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
		GPU_ClearColor(screen, SDL_Color{ 52, 55, 64, 255 });
		//GPU_ClearColor(screen, SDL_Color{ 0, 0, 0, 255 });
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
	case 5:
		/* Exit */
		{
			SDL_Event ev;
			ev.type = SDL_QUIT;
			SDL_PushEvent(&ev);
		}
		break;
	default:
		break;
	}
}