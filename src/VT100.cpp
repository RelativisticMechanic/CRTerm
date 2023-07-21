#include <iostream>
#include <conio.h>
#include "VT100.h"
#include "ConPTY.h"

void __cdecl outputListener(LPVOID term);

VT100::VT100(CRTermConfiguration* cfg)
{
	this->con = new Console(cfg);
	this->fg = con->default_fore_color;
	this->bg = con->default_back_color;
	this->CTRL_down = false;
	this->parser_state = VTSTATE_NORMAL;
	this->console_window = GetActiveWindow();
	HRESULT hr = CreatePseudoConsoleAndPipes(&hPC, &fromProgram, &toProgram, con->console_w, con->console_h);
	if (hr != S_OK)
	{
		fprintf(stderr, "Error while creating pseudoconsole.\n");
	}
	// Link Pipe to Listener thread
	output_listener_thread = { reinterpret_cast<HANDLE>(_beginthread(outputListener, 0, this)) };
	// Start the process
	std::wstring process_name = std::wstring(cfg->shell_command.begin(), cfg->shell_command.end());
	con->Puts("Terminal initialized.\n");
	con->Puts("Loading shell...\n");
	SpawnProcessinConsole((wchar_t*)process_name.c_str(), hPC, &cmd_process);
	// Wait
	Sleep(200);
}

// Takes an input, sets the relevant VT100 states
// and then outputs through the console provided to
// it on initialization.
void VT100::VT100Take(unsigned char c)
{
	switch (parser_state)
	{
	case VTSTATE_NORMAL:
		if (c == '\x1B')
		{
			parser_state = VTSTATE_ESC;
			stack_ptr = 0;
			for (int i = 0; i < VT100_ARG_STACK_SIZE; i++)
			{
				argument_stack[stack_ptr].value = 0;
				argument_stack[stack_ptr].empty = true;
			}
		}
		else
		{
			VT100Putc(c);
		}
		break;
	case VTSTATE_ESC:
		if (c == '[')
		{
			parser_state = VTSTATE_ATTR;
		}
		else if (c == ']')
		{
			parser_state = VTSTATE_CONTROL_STRING;
			control_string_idx = 0;
			for (int i = 0; i < VT100_STRING_SIZE; i++)
			{
				this->control_strings[i] = "";
			}
		}
		else
		{
			parser_state = VTSTATE_NORMAL;
			VT100Putc(c);
		}
		break;
	case VTSTATE_ATTR:
	case VTSTATE_PRIVATE:
		if (c == '?')
		{
			parser_state = VTSTATE_PRIVATE;
		}
		else
		{
			if (isdigit(c))
			{
				argument_stack[stack_ptr].value *= 10;
				argument_stack[stack_ptr].value += (c - '0');
				argument_stack[stack_ptr].empty = false;
			}
			else
			{
				if (stack_ptr < VT100_ARG_STACK_SIZE)
				{
					stack_ptr++;
				}
				argument_stack[stack_ptr].value = 0;
				argument_stack[stack_ptr].empty = true;
				if (parser_state != VTSTATE_PRIVATE)
					parser_state = VTSTATE_ENDVAL;
				else
					parser_state = VTSTATE_PRIVATE_ENDVAL;
			}
		}
		break;
	case VTSTATE_CONTROL_STRING:
		if (isdigit(c))
		{
			control_string_idx = (c - '0') % VT100_STRING_SIZE;
		}
		else
		{
			parser_state = VTSTATE_TAKE_STRING;
		}
		break;
	case VTSTATE_TAKE_STRING:
		if (c == '\x7')
		{
			parser_state = VTSTATE_NORMAL;
			// Set the window title to 0 string
			std::string window_title = "CRTerm.exe - " + this->control_strings[0];
			std::wstring window_title_wide = std::wstring(window_title.begin(), window_title.end());
			SetWindowText(this->console_window, (LPCWSTR)window_title_wide.c_str());
		}
		else
		{
			control_strings[control_string_idx] += c;
		}
		break;
	default:
		break;
	}

	if (parser_state == VTSTATE_ENDVAL)
	{
		if (c == ';')
		{
			parser_state = VTSTATE_ATTR;
		}
		else
		{
			switch (c)
			{
			case 'A':
				/* Cursor up P1 rows */
				if (argument_stack[0].empty)
				{
					con->SetCursor(con->cursor_x, con->cursor_y - 1);
				}
				else
				{
					con->SetCursor(con->cursor_x, con->cursor_y - argument_stack[0].value);
				}
				break;
			case 'B':
				/* Cursor down P1 rows */
				if (argument_stack[0].empty)
				{
					con->SetCursor(con->cursor_x, con->cursor_y + 1);
				}
				else
				{
					con->SetCursor(con->cursor_x, con->cursor_y + argument_stack[0].value);
				}
				break;
			case 'C':
				/* Cursor right P1 columns */
				if (argument_stack[0].empty)
				{
					con->SetCursor(con->cursor_x + 1, con->cursor_y);
				}
				else
				{
					con->SetCursor(con->cursor_x + argument_stack[0].value, con->cursor_y);
				}
				break;
			case 'D':
				/* Cursor left P1 columns */
				if (argument_stack[0].empty)
				{
					con->SetCursor(con->cursor_x - 1, con->cursor_y);
				}
				else
				{
					con->SetCursor(con->cursor_x - argument_stack[0].value, con->cursor_y);
				}
				break;
			case 'E':
				/* Cursor to first column of line P1 rows down from current */
				if (argument_stack[0].empty)
				{
					con->SetCursor(0, con->cursor_y + 1);
				}
				else
				{
					con->SetCursor(0, con->cursor_y + argument_stack[0].value);
				}
				break;
			case 'F':
				/* Cursor to first column of line P1 rows up from current */
				if (argument_stack[0].empty)
				{
					con->SetCursor(0, con->cursor_y - 1);
				}
				else
				{
					con->SetCursor(0, con->cursor_y - argument_stack[0].value);
				}
				break;
			case 'G':
				/* Cursor to column P1 */
				if (argument_stack[0].empty)
				{
					con->SetCursor(0, con->cursor_y);
				}
				else
				{
					con->SetCursor(argument_stack[0].value - 1, con->cursor_y);
				}
				break;
			case 'd':
				/* Cursor left P1 columns */
				break;
			case 'H':
			case 'f':
				/* Move cursor to row n, column m, 1-indexed */
				if (stack_ptr == 1 && argument_stack[stack_ptr].empty)
				{
					this->con->SetCursor(0, 0);
				}
				else if (stack_ptr == 2)
				{
					if (argument_stack[0].empty)
					{
						this->con->SetCursorY(0);
					}
					else
					{
						this->con->SetCursorY(argument_stack[0].value - 1);
					}

					if (argument_stack[1].empty)
					{
						this->con->SetCursorX(0);
					}
					else
					{
						this->con->SetCursorX(argument_stack[1].value - 1);
					}
				}
				break;
			case 'J':
				/* Clear part of the screen */
				if (argument_stack[0].empty)
				{
					this->con->ClearExt(con->cursor_x, con->cursor_y, con->console_w, con->console_h - 1);
				}
				else
				{
					int attr = argument_stack[0].value;
					switch (attr)
					{
					case 0:
						this->con->ClearExt(con->cursor_x, con->cursor_y, con->console_w, con->console_h - 1);
						break;
					case 1:
						this->con->ClearExt(0, 0, con->cursor_x, con->cursor_x);
						break;
					case 2:
						this->con->ClearExt(0, 0, con->console_w - 1, con->console_h - 1);
						break;
					default:
						break;
					}
				}
				break;
			case 'K':
				/* Erase line */
				if (argument_stack[0].empty)
				{
					con->ClearExt(con->cursor_x, con->cursor_y, con->console_w - 1, con->cursor_y);
				}
				else
				{
					switch (argument_stack[0].value)
					{
					case 0:
						con->ClearExt(con->cursor_x, con->cursor_y, con->console_w - 1, con->cursor_y);
						break;
					case 1:
						con->ClearExt(0, con->cursor_y, con->cursor_x, con->cursor_y);
						break;
					case 2:
						con->ClearExt(0, con->cursor_y, con->console_w - 1, con->cursor_y);
						break;
					default:
						break;
					}
				}
				break;
			case 'm':
				/* Set colour */
				for (int i = 0; i < stack_ptr; i++)
				{
					if (argument_stack[i].empty || argument_stack[i].value == 0)
					{
						this->fg = con->default_fore_color;
						this->bg = con->default_back_color;
					}
					else
					{
						int attr = argument_stack[i].value;
						if (attr == 1) // Increased intensity
						{
							this->fg |= 8;
						}
						else if (attr >= 30 && attr <= 37) // Set foreground color
						{
							this->fg = (attr - 30) % 8;
						}
						else if (attr >= 40 && attr <= 47) // Set background color
						{
							this->bg = (attr - 40) % 8;
						}

						// 39 and 49 set default fg and bg
						if (attr == 39)
						{
							this->fg = this->con->default_fore_color;
						}

						if (attr == 49)
						{
							this->bg = this->con->default_back_color;
						}

					}
				}
				break;
			case 'n':
				/* Report terminal parameters */
				if (argument_stack[0].value == 6)
				{
					std::string term_info = "\x1B[" + std::to_string(con->cursor_y + 1) + ';' + std::to_string(con->cursor_x + 1) + 'R';
					WriteFile(this->toProgram, term_info.c_str(), term_info.length(), NULL, NULL);
				}
				break;
			case 'S':
				/* Scroll Up */
				if (argument_stack[0].empty)
				{
					con->Scroll();
				}
				else
				{
					for (int i = 0; i < argument_stack[0].value; i++)
					{
						con->Scroll();
					}
				}
				break;

			}
			parser_state = VTSTATE_NORMAL;
		}
	}

	if (parser_state == VTSTATE_PRIVATE_ENDVAL)
	{
		switch (c)
		{
		case 'h':
			/* Show cursor */
			this->con->ShowCursor();
			break;
		case 'l':
			/* Hide cursor */
			this->con->HideCursor();
			break;
		}
		parser_state = VTSTATE_NORMAL;
	}
}

void VT100::VT100Putc(unsigned char c)
{
	this->con->PutCharExt(c, this->fg, this->bg);
}

void VT100::VT100HandleEvent(SDL_Event ev)
{
	switch (ev.type)
	{
	case SDL_TEXTINPUT:
		WriteFile(this->toProgram, &(ev.text.text[0]), 1, NULL, NULL);
		break;
	case SDL_KEYDOWN:
		if (special_key_map.find((int)ev.key.keysym.sym) != special_key_map.end())
		{
			std::string ansi_sequence = special_key_map[(int)ev.key.keysym.sym];
			WriteFile(this->toProgram, ansi_sequence.c_str(), ansi_sequence.length(), NULL, NULL);
		}

		if (ev.key.keysym.sym == SDLK_LCTRL || ev.key.keysym.sym == SDLK_RCTRL)
		{
			this->CTRL_down = true;
		}

		// If CTRL is down WITH C... send break.
		if (this->CTRL_down)
		{
			if (ev.key.keysym.sym == SDLK_c)
			{
				const char brk_char = '\x3';
				WriteFile(this->toProgram, &brk_char, 1, NULL, NULL);
			}
		}
		break;
	case SDL_KEYUP:
		if (ev.key.keysym.sym == SDLK_LCTRL || ev.key.keysym.sym == SDLK_RCTRL)
		{
			this->CTRL_down = false;
		}
		break;
	case SDL_QUIT:
		VT100Shutdown();
		break;
	default:
		break;
	}
}

void VT100::VT100Shutdown()
{
	// Sent exit to shell
	TerminateProcess(cmd_process.hProcess, 0);
	// Close handles
	ClosePseudoConsole(this->hPC);
	CloseHandle(this->fromProgram);
	CloseHandle(this->toProgram);
}
// Functions that listens to the program output.
void __cdecl outputListener(LPVOID term)
{
	const DWORD BUFF_SIZE { 8192 };
	char szBuffer[BUFF_SIZE]{};
	DWORD dwBytesWritten{};
	DWORD dwBytesRead{};
	BOOL fRead{ FALSE };
	VT100* vt100_term = (VT100*)term;
	do
	{
		// Read from the pipe
		fRead = ReadFile(vt100_term->fromProgram, szBuffer, BUFF_SIZE, &dwBytesRead, NULL);
		// Write received text to the Console
		for (int i = 0; i < dwBytesRead; i++)
		{
			if (szBuffer[i] == '\0')
				break;
			vt100_term->VT100Take(szBuffer[i]);
		}

	} while (fRead && dwBytesRead >= 0);
}