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

#define CONSOLE_MIN_LINES 500
#define CONSOLE_MAX_LINES 10000
#define CONSOLE_DEFAULT_LINES 1000

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
	/* Character-wise resolution of the console */
	int console_w, console_h;
	GPU_Image* console_font;
	GPU_Image* crt_background;
	int font_w, font_h;
	int cursor_x, cursor_y;
	/* Actual resolution of the console (in pixels) */
	int console_resolution_x, console_resolution_y;
	int default_fore_color, default_back_color;
	/* No. of millseconds after which the console must blink */
	int blink_interval;
	/* 
		For scrolling, console starts drawing from start_line*console_w, last_line is the latest start_line position
		For all time, start_line <= last_line. start_line is decremented when the user hits mousewheel up
		and Console->HistoryUp() is called. 

		maxlines is the maximum lines the console stores, beyond that, it starts to overwrite its old history.
		Generally, the lower limit for maxlines is 500, and upper limit is 10000 lines. 
	*/
	int start_line, last_line; 
	int maxlines;

	/* Selecting text into the terminal */
	bool is_selected;
	int selected_start_x = 0, selected_start_y = 0;
	int selected_end_x = 0, selected_end_y = 0;

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
	void SetSelection(bool selection, int start_x=0, int start_y=0, int end_x=0, int end_y=0);
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
