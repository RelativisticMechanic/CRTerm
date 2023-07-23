#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

#include "SDL_gpu.h"
#include "CRTermUI.h"

#define TITLE_BAR_HEIGHT 32
#define TITLE_BAR_ICON_SIZE 32

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