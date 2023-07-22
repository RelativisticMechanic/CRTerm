#include <iostream>
#include <string>

#include "ConfigEditor.h"
#include "HocevarPFD.h"

int selected_color = 0;

void ConfigEditor::Render(void)
{
 
    ImGui::Begin("Configuration Editor", 0, ImGuiWindowFlags_AlwaysAutoResize);

    /* Font Image */
    ImGui::Text("Font Image");
    ImGui::SameLine();
    ImGui::InputText("##font_image", &(cfg->bitmap_font_file));
    ImGui::SameLine();
    if (ImGui::Button("...##browse_image"))
    {
        auto selection = pfd::open_file("Select a file").result();
        if (!selection.empty())
            this->cfg->bitmap_font_file = selection[0];
    }

    ImGui::Text("Font Height");
    ImGui::SameLine();
    ImGui::InputInt("##font_height", &(cfg->font_height));

    ImGui::Text("Font Width");
    ImGui::SameLine();
    ImGui::InputInt("##font_width", &(cfg->font_width));

    ImGui::Text("Font Scale");
    ImGui::SameLine();
    ImGui::InputFloat("##font_scale", &(cfg->font_scale));

    ImGui::Text("Console Width");
    ImGui::SameLine();
    ImGui::InputInt("##console_width", &(cfg->console_width));

    ImGui::Text("Console Height");
    ImGui::SameLine();
    ImGui::InputInt("##console_height", &(cfg->console_height));

    ImGui::Text("CRT Warp");
    ImGui::SameLine();
    ImGui::InputFloat("##crt_warp", &(cfg->crt_warp));

    ImGui::Text("Blink Interval (ms)");
    ImGui::SameLine();
    ImGui::InputInt("##blink_interval", &(cfg->blink_interval));

    ImGui::Text("Default FG");
    ImGui::SameLine();
    ImGui::InputInt("##default_fg", &(cfg->default_fore_color));

    ImGui::Text("Default BG");
    ImGui::SameLine();
    ImGui::InputInt("##default_bg", &(cfg->default_back_color));

    ImGui::Text("Background");
    ImGui::SameLine();
    ImGui::InputText("##crt_background", &(cfg->crt_background_image));
    ImGui::SameLine();
    if (ImGui::Button("...##browse_crt"))
    {
        auto selection = pfd::open_file("Select a file").result();
        if (!selection.empty())
            this->cfg->crt_background_image = selection[0];
    }

    ImGui::Text("Text Shader");
    ImGui::SameLine();
    ImGui::InputText("##text_shader", &(cfg->shader_path_text));

    ImGui::Text("CRT Shader");
    ImGui::SameLine();
    ImGui::InputText("##crt_shader", &(cfg->shader_path_crt));

    ImGui::Text("Bell Sound");
    ImGui::SameLine();
    ImGui::InputText("##bell_sound", &(cfg->bell_sound));
    ImGui::SameLine();
    if (ImGui::Button("...##browse_bell"))
    {
        auto selection = pfd::open_file("Select a file").result();
        if (!selection.empty())
            this->cfg->bell_sound = selection[0];
    }

    ImGui::Text("Shell Command");
    ImGui::SameLine();
    ImGui::InputText("##shell_cmd", &(cfg->shell_command));

    ImGui::Text("Color Scheme");
    ImGui::Text("Set Color No: ");
    ImGui::SameLine();
    ImGui::InputInt("##color_setter", &selected_color);

    /* Limit the color to 15 */
    selected_color &= 0x0F;

    float rgb[3];
    rgb[0] = this->cfg->color_scheme[selected_color].r;
    rgb[1] = this->cfg->color_scheme[selected_color].g;
    rgb[2] = this->cfg->color_scheme[selected_color].b;
    if (ImGui::ColorPicker3("##cpicker", &rgb[0]))
    {
        this->cfg->color_scheme[selected_color].r = rgb[0];
        this->cfg->color_scheme[selected_color].r = rgb[1];
        this->cfg->color_scheme[selected_color].r = rgb[2];
    }

    if (ImGui::Button("Save Configuration##save_config"))
    {
        auto destination = pfd::save_file("Save Configuration", "config.json", { "JSON File", "*.json" }).result();
        if (!destination.empty())
        {
            this->cfg->Save(destination);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("Close##close_window"))
    {
        this->show = false;
    }

    ImGui::End();
}