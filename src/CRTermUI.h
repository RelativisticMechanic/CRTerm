#ifndef UIITEM_H
#define UIITEM_H

#include <vector>
#include "SDL_gpu.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "CRTermConfig.h"

class UIElement
{
public:
	UIElement()
	{
		this->show = true;
		this->window = NULL;
		this->context = NULL;
	}
	bool show;
	SDL_Window* window;
	void* context;
	virtual void Process(SDL_Event) {};
	virtual void Render() {};
};

class CRTermUIInstance
{
public:
	SDL_Window* sdl_window;
	void* gl_context;

	std::vector<UIElement*> elements;
	CRTermUIInstance(GPU_Target* screen);
	void HandleEvent(SDL_Event);
	void Render(void);
	void AddElement(UIElement* e);
};

#endif