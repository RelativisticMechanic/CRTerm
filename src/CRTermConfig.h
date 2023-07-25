/*
	CRTerm's configuration is loaded from a JSON file. The path to the JSON file is supposed
	to be described by a file called "default" in CRTerm.exe's directory which should just
	contain the path of the JSON (config / *.json).

	This is the class responsible for loading and saving the JSON structure.

	Nlohmann's JSON library is just awesome. 
*/

#ifndef CRTERM_CONFIG_H
#define CRTERM_CONFIG_H

#include <iostream>
#include <string>
#include "nlohmann/json.h"

typedef struct
{
	int r;
	int g; 
	int b;
} CRTermColor;

class CRTermConfiguration
{
public:
	std::string bitmap_font_file;
	std::string crt_background_image;
	std::string shader_path_text;
	std::string shader_path_crt;
	std::string shell_command;
	int font_width;
	int font_height;
	float font_scale;
	int console_width;
	int console_height;
	int default_fore_color;
	int default_back_color;
	int blink_interval;
	int maxlines;
	float crt_warp;
	CRTermColor color_scheme[16];

	CRTermConfiguration()
	{

	};

	CRTermConfiguration(std::string json_path);
	void Save(std::string filename);
};

/* Read the "default" file which contains the path to the default JSON location */
std::string GetDefaultConfigJSON();

#endif