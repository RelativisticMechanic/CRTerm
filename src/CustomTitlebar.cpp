#include "CustomTitleBar.h"
#include <Windows.h>

SDL_HitTestResult HitTestCallback(SDL_Window* Window, const SDL_Point* Area, void* Data)
{
    int Width, Height;
    SDL_GetWindowSize(Window, &Width, &Height);

    /* If the cursor is in the title bar region, we are dragable. */
    if (Area->y < TITLE_BAR_HEIGHT && Area->x < (Width - TITLE_BAR_ICON_SIZE * 3))
    {
        return SDL_HITTEST_DRAGGABLE;
    }

    return SDL_HITTEST_NORMAL;
}

CustomTitleBar::CustomTitleBar(GPU_Target* screen) : UIElement()
{
    this->window = SDL_GetWindowFromID(screen->context->windowID);
    SDL_SetWindowBordered(window, SDL_FALSE);
    SDL_SetWindowResizable(window, SDL_FALSE);
    SDL_SetWindowHitTest(window, HitTestCallback, 0);

    this->screen = screen;
    this->resolution_x = screen->w;
    this->resolution_y = screen->h;
    this->show = true;

    this->icon = GPU_LoadImage("ui/icon.png");
    GPU_SetImageFilter(this->icon, GPU_FILTER_NEAREST);
}
void CustomTitleBar::Render()
{
    /* Draw icon */
    GPU_Blit(this->icon, NULL, this->screen, TITLE_BAR_ICON_SIZE / 2, TITLE_BAR_ICON_SIZE / 2);

    /* Draw the window text */
    ImGui::SetNextWindowPos(ImVec2(32, 0));
    ImGui::SetNextWindowSize(ImVec2(this->resolution_x, TITLE_BAR_HEIGHT));
    ImGui::Begin("##title", 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    ImGuiTextBuffer title {};
    title.append(SDL_GetWindowTitle(this->window));
    ImGui::TextUnformatted(title.begin(), title.end());
    ImGui::PopStyleColor();
    ImGui::End();

    /* Draw the window buttons */
    ImGui::SetNextWindowPos(ImVec2(this->resolution_x - TITLE_BAR_ICON_SIZE * 3, 0));
    ImGui::SetNextWindowSize(ImVec2(TITLE_BAR_ICON_SIZE * 3, TITLE_BAR_HEIGHT));
    ImGui::Begin("##Window_Buttons", 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    
    /* Minimize button */
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 128, 0, 255));
    if (ImGui::Button(" _ "))
    {
        SDL_MinimizeWindow(this->window);
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    /* TODO: Maximize button doesn't do anything. */
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 128, 0, 255));
    ImGui::Button(" = ");
    ImGui::PopStyleColor();

    ImGui::SameLine();

    /* Unicode 'X', the close button */
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(128, 0, 0, 255));
    if (ImGui::Button(" \xC3\x97 "))
    {
        SDL_Event ev;
        ev.type = SDL_QUIT;
        SDL_PushEvent(&ev);
    }
    ImGui::PopStyleColor();

    ImGui::PopStyleColor();
    ImGui::End();
}