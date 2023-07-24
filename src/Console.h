/*
	This is the Console class. It is what does the fancy rendering of the terminal.
	The Console class right now implements a bitmap font, and has an attribute buffer
	and a character buffer. The attribute buffer holds the color information, and the
	character the ASCII characters.

	TODO: Implement true type and UTF-8 support.
*/
#ifndef CONSOLE_H
#define CONSOLE_H

#include <cstdint>
#include <string>
#include "CRTermConfig.h"
#include "SDL_gpu.h"

/* 
	Helper function to construct console attributes, 
	which are basically VGA attributes that store
	higher 4 bits as background color and lower 4 bits
	as foreground color
*/

#define CONSTRUCT_ATTRIBUTE(fcol,bcol) (((bcol) << 4) | (fcol))

class ConsoleColor
{
private:
	int r, g, b;
	float glsl_value[3];
public:
	ConsoleColor()
	{
		ConsoleColor(255, 255, 255);
	}
	ConsoleColor(int r, int g, int b)
	{
		this->r = r;
		this->g = g;
		this->b = b;
		this->glsl_value[0] = (float)r / (float)255.0;
		this->glsl_value[1] = (float)g / (float)255.0;
		this->glsl_value[2] = (float)b / (float)255.0;
	}

	float* returnArray()
	{
		return (float*) & (this->glsl_value);
	}

};

class Console
{
public:
	/* TODO: Some of these are better of in private. */
	unsigned char* buffer;
	unsigned char* attrib_buffer;
	int console_w, console_h;
	GPU_Image* console_font;
	GPU_Image* crt_background;
	int font_w, font_h;
	int cursor_x, cursor_y;
	int console_resolution_x, console_resolution_y;
	int default_fore_color, default_back_color;
	int blink_interval;
	int start_line, last_line; 
	int maxlines = 1000;
	std::string bell_wave_file;

	Console(CRTermConfiguration*);
	void PlaceChar(int x, int y, unsigned char c, int fore_color, int back_color);
	unsigned char ReadChar(int x, int y);
	void PutCharExt(unsigned char c, int fore_color, int back_color);
	void PutChar(unsigned char c);
	void Puts(std::string s);
	void Clear();
	void Scroll();
	void Render(GPU_Target* t, int x, int y, float scale);
	void SetCursor(int x, int y);
	void SetCursorX(int x);
	void SetCursorY(int y);
	void ShowCursor();
	void HideCursor();
	void PlayBell();
	void ClearExt(int fromx, int fromy, int tox, int toy);
	void HistoryUp();
	void HistoryDown();
	/* The 256 letters glyphs extracted from the font image */
	GPU_Image* char_blocks[256];

private:
	ConsoleColor color_scheme[16];
	uint32_t crt_shader_id, text_shader_id;
	GPU_ShaderBlock crt_shader_block, text_shader_block;
	/* 
		Before being scaled and shaded, the pure text is rendered here,
		it is shaded with the simple text shader that does the job of 
		setting the appropriate colors.
	*/
	GPU_Image* render_buffer;
	float crt_warp;
	float prev_time;
	float delta_time;
	float cursor_shadow_width;
	/* Varable to show the clock */
	float cursor_clock;
	/* This boolean is toggled every blink_rate milliseconds */
	bool show_cursor;

	void LimitCursor();
};

#endif
