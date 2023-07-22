/*
	This is the VT100 class, it does everything from creating a Console class, creating the pipes,
	launching the shell program.
	
	It is also what does the parsing of ESCAPE sequences received from
	shell programs. It then interprets these sequences and calls the methods from the Console
	class which is purely for rendering.

	It also sends data to the program from the input received from SDL2.
*/
#ifndef VT100_H
#define VT100_H

#include <Windows.h>
#include <string>
#include <map>
#include "SDL_gpu.h"
#include "Console.h"
#include "CRTermConfig.h"

#define VT100_ARG_STACK_SIZE 8
#define VT100_STRING_SIZE 8

typedef enum
{
	VTSTATE_NORMAL = 0, // 0
	VTSTATE_ESC, // 1
	VTSTATE_CONTROL_STRING, // 2
	VTSTATE_TAKE_STRING, // 3
	VTSTATE_PRIVATE, // 4
	VTSTATE_ATTR, // 5
	VTSTATE_ENDVAL,
	VTSTATE_PRIVATE_ENDVAL,
	VTSTATE_IGNORE_NEXT
} VT100ParseState;

typedef struct
{
	int value;
	bool empty;
} VT100Argument;

class VT100
{
public:
	Console* con;
	int fg;
	int bg;
	bool is_selected, is_dragging;
	int selected_start_x = 0;
	int selected_start_y = 0;
	int selected_end_x = 0;
	int selected_end_y = 0;

	VT100ParseState parser_state;
	VT100Argument argument_stack[VT100_ARG_STACK_SIZE];
	std::string control_strings[VT100_STRING_SIZE];
	int control_string_idx;
	int stack_ptr;

	
	HPCON hPC { INVALID_HANDLE_VALUE };
	HANDLE fromProgram { INVALID_HANDLE_VALUE };
	HANDLE toProgram { INVALID_HANDLE_VALUE };
	HANDLE output_listener_thread;
	PROCESS_INFORMATION cmd_process;
	HWND console_window;

	/* Special key map, maps SDL keycodes to VT100 sequences */
	std::unordered_map<int, std::string> special_key_map = {
		{ SDLK_RETURN, "\r" },
		{ SDLK_RETURN2, "\r"},
		{ SDLK_KP_ENTER, "\r" },
		{ SDLK_BACKSPACE, "\b" },
		{ SDLK_TAB, "\t" },
		{ SDLK_ESCAPE, "\x1B" },
		{ SDLK_UP, "\x1B[A" },
		{ SDLK_DOWN, "\x1B[B" },
		{ SDLK_RIGHT, "\x1B[C" },
		{ SDLK_LEFT, "\x1B[D" }
	};

	/* For sending ^C */
	bool CTRL_down;

	/* Required for mouse interactivty */
	float font_scale;

	VT100(CRTermConfiguration*);
	void VT100Take(unsigned char);
	void VT100Putc(unsigned char);
	void VT100HandleEvent(SDL_Event);
	void VT100Shutdown();
	void VT100Render(GPU_Target*);
	void VT100CopyToClipboard();
	void VT100PasteFromClipboard();

	inline void screenToConsoleCoords(int screenx, int screeny, int* conx, int* cony)
	{
		/* Translate from screen coordinates to console coordinates */
		float screen_unscaled_x = screenx / font_scale;
		float screen_unscaled_y = screeny / font_scale;

		// Next divide by the font_w to get the X coord, and font_h to get the y coord
		*conx = screen_unscaled_x / this->con->font_w;
		*cony = screen_unscaled_y / this->con->font_h;
	}

	inline void consoleToScreenCoords(int conx, int cony, int* screenx, int* screeny)
	{
		*screenx = conx * this->con->font_w * this->font_scale;
		*screeny = cony * this->con->font_h * this->font_scale;
	}

	inline void getConsoleMouseCoords(int* conx, int* cony)
	{
		int mouse_x = 0;
		int mouse_y = 0;
		SDL_GetMouseState(&mouse_x, &mouse_y);
		screenToConsoleCoords(mouse_x, mouse_y, conx, cony);
	}

	/* In case the user tries a funky way of selection */
	inline void orientSelectedCoords(void)
	{
		if (this->selected_start_x > this->selected_end_x)
		{
			std::swap(this->selected_start_x, this->selected_end_x);
		}
		if (this->selected_start_y > this->selected_end_y)
		{
			std::swap(this->selected_start_y, this->selected_end_y);
		}
	}
};

#endif