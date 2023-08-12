#include "CRTermUI.h"
#include "UITheme.h"

CRTermUIInstance::CRTermUIInstance(GPU_Target* screen)
{
	this->elements = std::vector<UIElement*>();
	this->sdl_window = SDL_GetWindowFromID(screen->context->windowID);
	this->gl_context = screen->context->context;
	ImGui::CreateContext();
	ImGui_ImplSDL2_InitForOpenGL(this->sdl_window, screen->context->context);
	SDL_GL_MakeCurrent(this->sdl_window, this->gl_context);
	const char* glsl_version = "#version 330";
	ImGui_ImplOpenGL3_Init(glsl_version);
	// Setup style
	SetUpImGuiTheme();
}

void CRTermUIInstance::AddElement(UIElement* e)
{
	e->window = this->sdl_window;
	e->context = this->gl_context;
	this->elements.push_back(e);
}

void CRTermUIInstance::HandleEvent(SDL_Event ev)
{
	ImGui_ImplSDL2_ProcessEvent(&ev);
	for (int i = 0; i < this->elements.size(); i++)
	{
		if (this->elements[i]->show)
		{
			this->elements[i]->Process(ev);
		}
	}
}

void CRTermUIInstance::Render(void)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(this->sdl_window);
	ImGui::NewFrame();

	/*ImGui::Begin("#FPS");
	ImGui::Text(std::to_string(ImGui::GetIO().Framerate).c_str());
	ImGui::End();*/

	for (int i = 0; i < this->elements.size(); i++)
	{
		if (this->elements[i]->show)
		{
			this->elements[i]->Render();
		}
	}
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}