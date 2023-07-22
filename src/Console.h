#ifndef CONSOLE_H
#define CONSOLE_H

#include <cstdint>
#include <string>
#include "CRTermConfig.h"
#include "SDL_gpu.h"

// Set lower 4 bits to text color
// And higher 4 bits to background color
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
	unsigned char* buffer;
	unsigned char* attrib_buffer;
	int console_w, console_h;
	GPU_Image* console_font;
	GPU_Image* crt_background;
	int font_w, font_h;
	int cursor_x, cursor_y;
	int console_resolution_x, console_resolution_y;
	int default_fore_color, default_back_color;
	int blink_interval = 0.5;
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
	// 256 letters
	GPU_Image* char_blocks[256];

private:
	void LimitCursor();
	ConsoleColor color_scheme[16];
	uint32_t crt_shader_id, text_shader_id;
	GPU_ShaderBlock crt_shader_block, text_shader_block;
	GPU_Image* render_buffer;
	float cursor_clock;
	float crt_warp;
	float prev_time;
	float delta_time;
	float cursor_shadow_width;
	bool show_cursor;
};

#endif
