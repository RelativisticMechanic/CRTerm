#include <Windows.h>
#include <iostream>
#include <fstream>
#include "CRTermConfig.h"

CRTermConfiguration::CRTermConfiguration(std::string json_path)
{
	nlohmann::json configuration_data;
	std::ifstream json_file(json_path);
	try
	{
		configuration_data = nlohmann::json::parse(json_file);
		this->font_height = configuration_data.at("font_height");
		this->font_width = configuration_data.at("font_width");
		this->font_scale = configuration_data.at("font_scale");
		this->bitmap_font_file = configuration_data.at("font");
		this->console_width = configuration_data.at("console_width");
		this->console_height = configuration_data.at("console_height");
		this->crt_background_image = configuration_data.at("background");
		this->shader_path_text = configuration_data.at("text_shader");
		this->shader_path_crt = configuration_data.at("crt_shader");
		this->bell_sound = configuration_data.at("bell");
		this->shell_command = configuration_data.at("shell_command");
		this->blink_interval = configuration_data.at("blink_interval");
		this->default_fore_color = configuration_data.at("default_fg");
		this->default_back_color = configuration_data.at("default_bg");
		
		int i = 0;
		for (auto& color : configuration_data.at("color_scheme"))
		{
			if (i > 15)
				break;

			this->color_scheme[i].r = color[0];
			this->color_scheme[i].g = color[1];
			this->color_scheme[i].b = color[2];
			i += 1;
		}
	}
	catch(nlohmann::json::parse_error& error)
	{
		std::string err = error.what();
		std::wstring errW = std::wstring(err.begin(), err.end());
		MessageBox(GetActiveWindow(), (LPCWSTR)errW.c_str(), L"Error parsing default.json", MB_OK | MB_ICONERROR);
		exit(-1);
	}
	catch (...)
	{
		MessageBox(GetActiveWindow(), L"An exception occurred while loading default.json", L"Error loading default.json", MB_OK | MB_ICONERROR);
	}
}