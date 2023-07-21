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
	VTSTATE_PRIVATE_ENDVAL
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

	VT100ParseState parser_state;
	VT100Argument argument_stack[VT100_ARG_STACK_SIZE];
	std::string control_strings[VT100_STRING_SIZE];
	int control_string_idx;
	int stack_ptr;

	//HANDLE input_handle;
	HPCON hPC { INVALID_HANDLE_VALUE };
	HANDLE fromProgram { INVALID_HANDLE_VALUE };
	HANDLE toProgram { INVALID_HANDLE_VALUE };
	HANDLE output_listener_thread;
	PROCESS_INFORMATION cmd_process;
	HWND console_window;

	// Special key map
	std::unordered_map<int, std::string> special_key_map = {
		{ SDLK_RETURN, "\r" },
		{ SDLK_RETURN2, "\r"},
		{ SDLK_BACKSPACE, "\b" },
		{ SDLK_TAB, "\t" },
		{ SDLK_ESCAPE, "\x1B" },
		{ SDLK_UP, "\x1B[A" },
		{ SDLK_DOWN, "\x1B[B" },
		{ SDLK_RIGHT, "\x1B[C" },
		{ SDLK_LEFT, "\x1B[D" }
	};

	// For sending control sequences
	bool CTRL_down;

	VT100(CRTermConfiguration*);
	void VT100Take(unsigned char);
	void VT100Putc(unsigned char);
	void VT100HandleEvent(SDL_Event);
	void VT100Shutdown();
};

#endif