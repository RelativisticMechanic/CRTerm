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
#include "SDL_gpu.h"

#include "CRTermConfig.h"
#include "ConsoleFont.h"
#include "Timer.h"

#define CONSOLE_MIN_LINES 500
#define CONSOLE_MAX_LINES 10000
#define CONSOLE_DEFAULT_LINES 1000

/* 
	Helper function to construct console attributes, 
	which are basically VGA attributes that store
	higher 8 bits as background color and lower 8 bits
	as foreground color.
*/

/*
	The attribute is a 32-bit structure that looks like this:
	BITS 0-7: 8-bit forecolor
	BITS 8-15: 8-bit backcolor
	BITS 16-31: Unused

	Usually, unless specified the 16-31 bits are zeroed for normal operation.
*/

#define CONSTRUCT_ATTRIBUTE(fcol,bcol) ((((bcol) << 8) | (fcol)))

/* The PREPRARE_REDRAW keyword is called whenever we do something that will require the console text buffer to be redrawn. */
#define PREPARE_REDRAW (redraw_console = true)

/*
	Class to abstract color stuff.
*/
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
	ConsoleChar* buffer;
	ConsoleAttrib* attrib_buffer;
	/* Character-wise resolution of the console */
	int console_w, console_h;
	/* Font interface to be used by the console */
	ConsoleFont* console_font;
	/* Background image and noise texture for the shaders */
	GPU_Image* crt_background = NULL;
	GPU_Image* noise_texture;
	/* Font width and height (in pixels) */
	int font_w, font_h;
	/* Cursor position (in terms of characters) */
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
	/* Start of selection coordinates */
	int selected_start_x = 0, selected_start_y = 0;
	/* End of selection coordinates */
	int selected_end_x = 0, selected_end_y = 0;

	/* Wrap Around (DECAWM) */
	bool wrap_around = true;

	Console(CRTermConfiguration*, ConsoleFont*);
	void PlaceChar(int x, int y, ConsoleChar c, int fore_color, int back_color);
	ConsoleChar ReadChar(int x, int y);
	void PutCharExt(ConsoleChar c, int fore_color, int back_color);
	void PutChar(ConsoleChar c);
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
	void EnableWrapAround();
	void DisableWrapAround();
	void Redraw(void);
	void SetSelection(bool selection, int start_x=0, int start_y=0, int end_x=0, int end_y=0);

private:
	/* Color palette, first 16 colors from JSON, rest 240 from XTerm256Palette.cpp */
	ConsoleColor colors[256];

	/*
		Common Shader	- Default Shader (not really used but only there for debugging purposes)
		Text Shader		- Applied when rendering into internal render_buffer, sets the text color.
		CRT Shader		- Applied when rendering on the screen, takes the render_buffer, upscales and distorts it.
	*/
	uint32_t crt_shader_id, text_shader_id, common_shader_id;
	GPU_ShaderBlock crt_shader_block, text_shader_block, common_shader_block;
	/* 
		Before being scaled and shaded, the pure text is rendered here,
		it is shaded with the simple text shader that does the job of 
		setting the appropriate colors.
	*/
	GPU_Image* render_buffer;

	/* Older frame used for the burn in effect */
	GPU_Image* older_frame;

	/* Timer that allows this operation */
	Timer old_frame_timer;

	/* Last burn in time */
	float last_burn_in_time = 0.0f;

	/* Boolean set to true by PREPARE_REDRAW, then set to false after Console::Redraw() is called. */
	bool redraw_console = true;

	/* CRT warp factor passed to shader. */
	float crt_warp;

	/* Internal function used to keep cursor in limits. */
	void LimitCursor();
};

#endif
