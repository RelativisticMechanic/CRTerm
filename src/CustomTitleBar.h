/*
	This contains code for the custom titlebar for the program.
*/
#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include "SDL_gpu.h"
#include "CRTermUI.h"

#undef CRTERM_CUSTOM_TITLE_BAR

#ifdef CRTERM_CUSTOM_TITLE_BAR
#define TITLE_BAR_HEIGHT 32
/* Width of the custom side bars */
#define SIDES_WIDTH 8
/* Size of the icon shown in terminal */
#define TITLE_BAR_ICON_SIZE 32
#else
#define TITLE_BAR_HEIGHT 0
#define SIDES_WIDTH 0
#define TITLE_BAR_ICON_SIZE 0
#endif
SDL_HitTestResult HitTestCallback(SDL_Window* Window, const SDL_Point* Area, void* Data);

class CustomTitleBar : public UIElement
{
public:
	int resolution_x, resolution_y;
	GPU_Target* screen;
	SDL_Window* window;
	GPU_Image* icon;
	CustomTitleBar(GPU_Target*);
	void Render() override;
};

#endif