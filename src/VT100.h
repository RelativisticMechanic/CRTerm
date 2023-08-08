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
#include "ConsoleFont.h"
#include "CRTermConfig.h"

#define VT100_ARG_STACK_SIZE 8

typedef enum
{
	VTSTATE_NORMAL = 0,
	VTSTATE_ESC,
	VTSTATE_CONTROL_STRING,
	VTSTATE_TAKE_STRING,
	VTSTATE_END_STRING,
	VTSTATE_PRIVATE,
	VTSTATE_ATTR,
	VTSTATE_ENDVAL,
	VTSTATE_PRIVATE_ENDVAL,
	VTSTATE_CONTROL_ENDVAL,
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
	GPU_Target* render_target;
	int fg;
	int bg;

	/* 
		Following are the VT100 state variables that will be
		used by the VT100_HandlEvent() 
	*/

	/* For bracketed paste mode */
	bool bracketed_mode = false; 
	/* For alternative keypad mode */
	bool alternate_keypad_mode = false;
	/* For DECKAM */
	bool keyboard_disabled = false;
	/* For reporting focus change */
	bool report_focus_change = false;

	bool is_selected, is_dragging;
	int selected_start_x = 0;
	int selected_start_y = 0;
	int selected_end_x = 0;
	int selected_end_y = 0;

	int screen_offsetx = 0;
	int screen_offsety = 0;

	VT100ParseState parser_state;
	VT100Argument argument_stack[VT100_ARG_STACK_SIZE];
	std::string title;
	int control_string_idx;
	int stack_ptr;

	SDL_Window* sdl_window;
	
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
		/* TODO: cmd takes 0x7F instead of 0x08 as backspace. 0x08 on cmd clears the whole word. Weird. */
		{ SDLK_BACKSPACE, "\x7F" },
		{ SDLK_TAB, "\t" },
		{ SDLK_ESCAPE, "\x1B" },
		{ SDLK_UP, "\x1B[A" },
		{ SDLK_DOWN, "\x1B[B" },
		{ SDLK_RIGHT, "\x1B[C" },
		{ SDLK_LEFT, "\x1B[D" },
		{ SDLK_DELETE, "\b" },
		{ SDLK_HOME, "\x1B[1~" },
		{ SDLK_INSERT, "\x1B[2~"},
		{ SDLK_DELETE, "\x1B[3~"},
		{ SDLK_END, "\x1B[4~" },
		{ SDLK_PAGEUP, "\x1B[5~" },
		{ SDLK_PAGEDOWN, "\x1B[6~" },
		{ SDLK_F1, "\x1B[11~" },
		{ SDLK_F2, "\x1B[12~" },
		{ SDLK_F3, "\x1B[13~" },
		{ SDLK_F4, "\x1B[14" },
		{ SDLK_F5, "\x1B[15~" },
		{ SDLK_F6, "\x1B[17~" },
		{ SDLK_F7, "\x1B[18~" },
		{ SDLK_F8, "\x1B[19~" },
		{ SDLK_F9, "\x1B[20~" },
		{ SDLK_F10, "\x1B[21~" },
		{ SDLK_F11, "\x1B[23~" },
		{ SDLK_F12, "\x1B[24~"}
	};

	/* Control Key map, for stuff like ^C, ^O, etc. */
	std::unordered_map<int, std::string> control_key_map = {
		{ SDLK_SPACE, "\x0" },
		{ SDLK_a, "\x1" },
		{ SDLK_b, "\x2" },
		{ SDLK_c, "\x3" },
		{ SDLK_d, "\x4" },
		{ SDLK_e, "\x5" },
		{ SDLK_f, "\x6" },
		{ SDLK_g, "\x7" },
		{ SDLK_h, "\x8" },
		{ SDLK_i, "\x9" },
		{ SDLK_j, "\xA" },
		{ SDLK_k, "\xB" },
		{ SDLK_l, "\xC" }, 
		{ SDLK_m, "\xD" },
		{ SDLK_n, "\xE" },
		{ SDLK_o, "\xF" },
		{ SDLK_p, "\x10" },
		{ SDLK_q, "\x11" },
		{ SDLK_r, "\x12" },
		{ SDLK_s, "\x13" },
		{ SDLK_t, "\x14" },
		{ SDLK_u, "\x15" },
		{ SDLK_v, "\x16" },
		{ SDLK_w, "\x17" },
		{ SDLK_x, "\x18" },
		{ SDLK_y, "\x19" },
		{ SDLK_z, "\x1A" }
	};

	/* For sending ^C */
	bool CTRL_down;

	/* Required for mouse interactivty */
	float font_scale;

	/* How many bytes to read into unicode? Right now, we just skip them and print an ASCII block at the end. */
	int utf8_bytes_left = 0;
	ConsoleChar utf8_char;

	VT100(CRTermConfiguration*, ConsoleFont*, GPU_Target*);
	void VT100Take(unsigned char);
	void VT100Putc(unsigned char);
	void VT100Puts(std::string s);
	void VT100Send(std::string);
	void VT100HandleEvent(SDL_Event);
	void VT100Shutdown();
	void VT100Render();
	void VT100CopyToClipboard();
	void VT100PasteFromClipboard();

	inline void screenToConsoleCoords(int screenx, int screeny, int* conx, int* cony)
	{
		/* Translate from screen coordinates to console coordinates */
		float screen_unscaled_x = (screenx - screen_offsetx) / font_scale;
		float screen_unscaled_y = (screeny - screen_offsety) / font_scale;

		// Next divide by the font_w to get the X coord, and font_h to get the y coord
		*conx = screen_unscaled_x / this->con->font_w;
		*cony = screen_unscaled_y / this->con->font_h;
	}

	inline void consoleToScreenCoords(int conx, int cony, int* screenx, int* screeny)
	{
		*screenx = (int)((double)screen_offsetx + (double)conx * (double)this->con->font_w * this->font_scale);
		*screeny = (int)((double)screen_offsety + (double)cony * (double)this->con->font_h * this->font_scale);
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