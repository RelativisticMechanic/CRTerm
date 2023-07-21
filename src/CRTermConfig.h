#ifndef CRTERM_CONFIG_H
#define CRTERM_CONFIG_H

#include <string>
#include "NlohmannJSON.h"

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
	std::string bell_sound;
	std::string shell_command;
	int font_width;
	int font_height;
	float font_scale;
	int console_width;
	int console_height;
	int default_fore_color;
	int default_back_color;
	int blink_interval;
	float crt_warp;
	CRTermColor color_scheme[16];

	CRTermConfiguration(std::string json_path);
};

#endif