#include "CustomTitleBar.h"
#include <Windows.h>

SDL_HitTestResult HitTestCallback(SDL_Window* Window, const SDL_Point* Area, void* Data)
{
    int Width, Height;
    SDL_GetWindowSize(Window, &Width, &Height);

    /* If the cursor is in the title bar region, we are dragable. */
    if (Area->y < TITLE_BAR_HEIGHT && Area->x < (Width - TITLE_BAR_ICON_SIZE * 2))
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

    this->icon = GPU_LoadImage("ui/logo.png");
    GPU_SetImageFilter(this->icon, GPU_FILTER_NEAREST);
}
void CustomTitleBar::Render()
{
    /* Draw icon, 32x32 */
    GPU_Blit(this->icon, NULL, this->screen, TITLE_BAR_ICON_SIZE / 2, TITLE_BAR_ICON_SIZE / 2);

    ImGui::SetNextWindowPos(ImVec2(32, 0));
    ImGui::SetNextWindowSize(ImVec2(this->resolution_x, TITLE_BAR_HEIGHT));
    ImGui::Begin("##title", 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    static ImGuiTextBuffer title {};
    title.clear();
    title.append(SDL_GetWindowTitle(this->window));
    ImGui::TextUnformatted(title.begin(), title.end());
    ImGui::PopStyleColor();
    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(this->resolution_x - TITLE_BAR_ICON_SIZE * 2, 0));
    ImGui::SetNextWindowSize(ImVec2(TITLE_BAR_ICON_SIZE * 2, TITLE_BAR_HEIGHT));
    ImGui::Begin("##XButton", 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
    if (ImGui::Button("_"))
    {
        SDL_MinimizeWindow(this->window);
    }
    ImGui::SameLine();
    if (ImGui::Button("X"))
    {
        SDL_Event ev;
        ev.type = SDL_QUIT;
        SDL_PushEvent(&ev);
    }
    ImGui::PopStyleColor();
    ImGui::End();
}