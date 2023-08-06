#include <Windows.h>
#include <iostream>
#include <fstream>
#include "CRTerm.h"
#include "CRTermConfig.h"

std::string GetDefaultConfigJSON()
{
	FILE* fp;
	fopen_s(&fp, "default", "r");
	if (!fp)
		return "";

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* s = (char*)calloc(size + 1, sizeof(char));

	if (!s)
		return "";

	fread(s, sizeof(char), size, fp);

	fclose(fp);
	std::string json_file_name = std::string(s);
	return json_file_name;
}

CRTermConfiguration::CRTermConfiguration(std::string json_path)
{
	nlohmann::json configuration_data;
	std::ifstream json_file(json_path);
	try
	{
		configuration_data = nlohmann::json::parse(json_file);
		this->bitmap_font_file = configuration_data.at("font");
		/* If font is a PNG, expect font_height, font_width */
		if (endsWith(this->bitmap_font_file, ".png"))
		{
			this->font_height = configuration_data.at("font_height");
			this->font_width = configuration_data.at("font_width");
		}
		else
		{
			this->font_height = 0;
			this->font_width = 0;
		}
		this->font_scale = configuration_data.at("font_scale");
		this->console_width = configuration_data.at("console_width");
		this->console_height = configuration_data.at("console_height");
		this->crt_background_image = configuration_data.at("background");
		this->shader_path_text = configuration_data.at("text_shader");
		this->shader_path_crt = configuration_data.at("crt_shader");
		this->shell_command = configuration_data.at("shell_command");
		this->blink_interval = configuration_data.at("blink_interval");
		this->default_fore_color = configuration_data.at("default_fg");
		this->default_back_color = configuration_data.at("default_bg");
		this->crt_warp = configuration_data.at("crt_warp");
		this->maxlines = configuration_data.at("maxlines");
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
	catch (nlohmann::json::other_error& error)
	{
		std::string err = error.what();
		std::wstring errW = std::wstring(err.begin(), err.end());
		MessageBox(GetActiveWindow(), (LPCWSTR)errW.c_str(), L"Error loading default.json", MB_OK | MB_ICONERROR);
	}
	catch (nlohmann::json::type_error& error)
	{
		std::string err = error.what();
		std::wstring errW = std::wstring(err.begin(), err.end());
		MessageBox(GetActiveWindow(), (LPCWSTR)errW.c_str(), L"Error loading default.json", MB_OK | MB_ICONERROR);
	}
	catch (...)
	{
		MessageBox(GetActiveWindow(), L"An unknown error occurred", L"Error loading default.json", MB_OK | MB_ICONERROR);
	}
}

void CRTermConfiguration::Save(std::string filename)
{
	nlohmann::json output_json;

	output_json["font_height"] = this->font_height;
	output_json["font_width"] = this->font_width;
	output_json["font_scale"] = this->font_scale;
	output_json["font"] = this->bitmap_font_file;
	output_json["console_width"] = this->console_width;
	output_json["console_height"] = this->console_height;
	output_json["background"] = this->crt_background_image;
	output_json["text_shader"] = this->shader_path_text;
	output_json["crt_shader"] = this->shader_path_crt;

	output_json["crt_shader"] = this->shader_path_crt;
	output_json["shell_command"] = this->shell_command;
	output_json["blink_interval"] = this->blink_interval;
	output_json["default_fg"] = this->default_fore_color;
	output_json["default_bg"] = this->default_back_color;
	output_json["crt_warp"] = this->crt_warp;
	output_json["maxlines"] = this->maxlines;

	int color_scheme_arr[16][3];

	for (int i = 0; i < 16; i++)
	{
		color_scheme_arr[i][0] = color_scheme[i].r;
		color_scheme_arr[i][1] = color_scheme[i].g;
		color_scheme_arr[i][2] = color_scheme[i].b;
	}

	output_json["color_scheme"] = color_scheme_arr;

	std::ofstream output(filename);
	output << std::setw(4) << output_json;
	output.close();
}