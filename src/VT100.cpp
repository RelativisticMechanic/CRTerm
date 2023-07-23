#include <iostream>
#include <conio.h>
#include <Windows.h>
#include "VT100.h"
#include "ConPTY.h"
#include "Win32ClipBoard.h"

void __cdecl outputListener(LPVOID term);

VT100::VT100(CRTermConfiguration* cfg)
{
	con = new Console(cfg);
	this->fg = con->default_fore_color;
	this->bg = con->default_back_color;
	this->bracketed_mode = true;
	this->CTRL_down = false;
	this->is_selected = false;
	this->parser_state = VTSTATE_NORMAL;
	console_window = GetActiveWindow();

	this->font_scale = cfg->font_scale;
	HRESULT hr = CreatePseudoConsoleAndPipes(&hPC, &fromProgram, &toProgram, con->console_w, con->console_h);
	if (hr != S_OK)
	{
		fprintf(stderr, "Error while creating pseudoconsole.\n");
	}
	/* Link Pipe to Listener thread */
	output_listener_thread = { reinterpret_cast<HANDLE>(_beginthread(outputListener, 0, this)) };
	/* Start the process */
	std::wstring process_name = std::wstring(cfg->shell_command.begin(), cfg->shell_command.end());
	con->Puts("Terminal initialized.\n");
	con->Puts("Loading shell...");
	SpawnProcessinConsole((wchar_t*)process_name.c_str(), hPC, &cmd_process);
	/* Wait for it to spawn */
	Sleep(500);
}

/* 
This is the VT100 parser, it takes one character at a time,
sends the terminal states and outputs non escaped characters.

The thread "outputListener" (defined at the bottom) sends data
to this from the child process' (usually a shell) pipe.
*/
void VT100::VT100Take(unsigned char c)
{
	switch (parser_state)
	{
	case VTSTATE_IGNORE_NEXT:
		parser_state = VTSTATE_NORMAL;
		break;
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
				control_strings[i] = "";
			}
		}
		else
		{
			std::cout << "Unexpected identifier after VT_ESCAPE: " << c << std::endl;
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
			std::string window_title = "CRTerm.exe - " + control_strings[0];
			std::wstring window_title_wide = std::wstring(window_title.begin(), window_title.end());
			SetWindowText(console_window, (LPCWSTR)window_title_wide.c_str());
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
			case 'H':
			case 'f':
				/* Move cursor to row n, column m, 1-indexed */
				if (stack_ptr == 1 && argument_stack[stack_ptr].empty)
				{
					con->SetCursor(0, 0);
				}
				else if (stack_ptr == 2)
				{
					if (argument_stack[0].empty)
					{
						con->SetCursorY(0);
					}
					else
					{
						con->SetCursorY(argument_stack[0].value - 1);
					}

					if (argument_stack[1].empty)
					{
						con->SetCursorX(0);
					}
					else
					{
						con->SetCursorX(argument_stack[1].value - 1);
					}
				}
				break;
			case 'J':
				/* Clear part of the screen */
				if (argument_stack[0].empty)
				{
					con->ClearExt(con->cursor_x, con->cursor_y, con->console_w - 1, con->console_h - 1);
				}
				else
				{
					int attr = argument_stack[0].value;
					switch (attr)
					{
					case 0:
						con->ClearExt(con->cursor_x, con->cursor_y, con->console_w - 1, con->console_h - 1);
						break;
					case 1:
						con->ClearExt(0, 0, con->cursor_x, con->cursor_y);
						break;
					case 2:
						con->ClearExt(0, 0, con->console_w - 1, con->console_h - 1);
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
						/* Make bright if 1 */
						if (attr == 1)
						{
							this->fg |= 8;
							/* Don't make background bright if default color, as it is blinding after the glow CRT shader LOL */
							if(this->bg != con->default_back_color)
								this->bg |= 8;
						}
						/* FG is between 30-37 */
						else if (attr >= 30 && attr <= 37)
						{
							this->fg = (attr - 30) % 8;
						}
						/* BG is between 40-47 */
						else if (attr >= 40 && attr <= 47) 
						{
							this->bg = (attr - 40) % 8;
						}

						/* 39 and 49 set default fg and bg */
						if (attr == 39)
						{
							this->fg = con->default_fore_color;
						}

						else if (attr == 49)
						{
							this->bg = con->default_back_color;
						}

						else if (attr == 38)
						{
							if (i < VT100_ARG_STACK_SIZE - 1)
							{
								i += 1;
								if (argument_stack[i].value == 5)
								{
									/* ISO 8613-3 CSI 38;5;Ps - Set FG to PS */
									if (i < VT100_ARG_STACK_SIZE - 1)
									{
										i += 1;
										this->fg = argument_stack[i].value;
									}
								}
							}
						}
						else if (attr == 48)
						{
							if (i < VT100_ARG_STACK_SIZE - 1)
							{
								i += 1;
								if (argument_stack[i].value == 5)
								{
									/* ISO 8613-3 CSI 48;5;Ps - Set BG to PS */
									if (i < VT100_ARG_STACK_SIZE - 1)
									{
										i += 1;
										this->bg = argument_stack[i].value;
									}
								}
							}
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
			case 'X':
				/* Erase n chars to the right */
				if (argument_stack[0].empty)
				{
					con->PlaceChar(con->cursor_x + 1, con->cursor_y, ' ', con->default_fore_color, con->default_back_color);
				}
				else
				{
					for (int i = 0; i < this->argument_stack[0].value; i++)
					{
						con->PlaceChar(con->cursor_x + i, con->cursor_y, ' ', con->default_fore_color, con->default_back_color);
					}
				}
				break;
			case '%':
				parser_state = VTSTATE_IGNORE_NEXT;
				break;
			default:
				std::cout << "Unimplemented sequence: " << c << std::endl;
				break;
			}
			if(parser_state != VTSTATE_IGNORE_NEXT)
				parser_state = VTSTATE_NORMAL;
		}
	}

	if (parser_state == VTSTATE_PRIVATE_ENDVAL)
	{
		switch (c)
		{
		case 'h':
			/* CSI ? 25 h Show cursor */
			if (this->argument_stack[0].value == 25)
			{
				con->ShowCursor();
			}
			/* CST ? 2004 h Turn ON bracketed paste */
			if (this->argument_stack[0].value == 2004)
			{
				this->bracketed_mode = true;
			}
			break;
		case 'l':
			/* CSI ? 25 l Hide cursor */
			if (this->argument_stack[0].value == 25)
			{
				con->HideCursor();
			}
			/* CST ? 2004 h Turn OFF bracketed paste */
			if (this->argument_stack[0].value == 2004)
			{
				this->bracketed_mode = false;
			}
			break;
		case '$':
			/* CSI ? ARGS $p - Unimplemented */
			parser_state = VTSTATE_IGNORE_NEXT;
			break;
		}
		if (parser_state != VTSTATE_IGNORE_NEXT)
			parser_state = VTSTATE_NORMAL;
	}
}

void VT100::VT100Putc(unsigned char c)
{
	con->PutCharExt(c, this->fg, this->bg);
}

void VT100::VT100HandleEvent(SDL_Event ev)
{
	switch (ev.type)
	{
	case SDL_TEXTINPUT:
		/* Send text input as is, SDLs text input is ASCII compliant */
		WriteFile(this->toProgram, &(ev.text.text[0]), 1, NULL, NULL);
		break;
	case SDL_KEYDOWN:
		/* Send special keys */
		if (special_key_map.find((int)ev.key.keysym.sym) != special_key_map.end())
		{
			std::string ansi_sequence = special_key_map[(int)ev.key.keysym.sym];
			WriteFile(this->toProgram, ansi_sequence.c_str(), ansi_sequence.length(), NULL, NULL);
		}

		if (ev.key.keysym.sym == SDLK_LCTRL || ev.key.keysym.sym == SDLK_RCTRL)
		{
			this->CTRL_down = true;
		}

		/* If CTRL is down WITH C... send break. */
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
	case SDL_MOUSEBUTTONDOWN:
		if (ev.button.button == SDL_BUTTON_LEFT)
		{
			getConsoleMouseCoords(&(this->selected_start_x), &(this->selected_start_y));
			getConsoleMouseCoords(&(this->selected_end_x), &(this->selected_end_y));
			this->is_dragging = true;
		}
		break;
	case SDL_MOUSEMOTION:
		if (this->is_dragging)
		{
			getConsoleMouseCoords(&(this->selected_end_x), &(this->selected_end_y));
		}
		break;
	case SDL_MOUSEBUTTONUP:
		if (ev.button.button == SDL_BUTTON_LEFT)
		{
			/* If the start and end position are not the same, then only select. */
			if (this->selected_end_x != this->selected_start_x || this->selected_start_y != this->selected_end_y)
			{
				this->is_selected = true;
			}
			else
			{
				this->is_selected = false;
			}
			this->is_dragging = false;
		}
		break;
	case SDL_QUIT:
		VT100Shutdown();
		break;
	default:
		break;
	}
}

void VT100::VT100Render(GPU_Target* t)
{
	con->Render(t, 0, 0, this->font_scale);
	GPU_DeactivateShaderProgram();

	/* Draw overlay above the selected text if it is selected or dragging */
	if (is_selected || is_dragging)
	{
		/* In case the user has selected it from right to left, orient the selection coords */
		orientSelectedCoords();
		for (int x = this->selected_start_x; x <= this->selected_end_x; x++)
		{
			for (int y = this->selected_start_y; y <= this->selected_end_y; y++)
			{
				int sx, sy;
				consoleToScreenCoords(x, y, &sx, &sy);
				GPU_RectangleFilled(t, sx, sy, sx + con->font_w * this->font_scale, sy + con->font_h * this->font_scale, SDL_Color{255, 255, 255, 128});
			}
		}
	}
}
void VT100::VT100Shutdown()
{
	/* Terminate child process */
	TerminateProcess(cmd_process.hProcess, 0);
	/* Close all handles */
	ClosePseudoConsole(this->hPC);
	CloseHandle(this->fromProgram);
	CloseHandle(this->toProgram);
}

void VT100::VT100CopyToClipboard()
{
	/* If selected and user presses right click, copy to clipboard */
	if (this->is_selected)
	{
		std::string result = "";
		/* In case the user has selected it from right to left, orient the selection coords */
		orientSelectedCoords();
		for (int x = this->selected_start_x; x <= this->selected_end_x; x++)
		{
			for (int y = this->selected_start_y; y <= this->selected_end_y; y++)
			{
				result += con->ReadChar(x, y);
			}
		}
		CopyToClipboard(result);
		this->is_selected = false;
	}
}

void VT100::VT100PasteFromClipboard()
{
	/* Take data from clipboard also escape the string as per ANSI if in bracketed mode */
	std::string clipboard = GetFromClipboard();
	if (bracketed_mode)
	{
		clipboard = "\x1B[200~" + clipboard + "\x1B[201~";
	}

	bool permission = false;

	if (!bracketed_mode)
	{
		/* Show warning to user if pasting in unbracketed mode */
		int result = MessageBox(this->console_window, L"The terminal is in unbracketed mode. Pasting unescaped text could be dangerous. Do it?", L"CRTerm.exe", MB_OKCANCEL | MB_ICONWARNING);
		if (result != IDCANCEL)
		{
			permission = true;
		}
	}
	else
	{
		permission = true;
	}

	/* Send to pipe! */
	if(permission)
		WriteFile(this->toProgram, clipboard.c_str(), clipboard.length(), NULL, NULL);
}

/* This function is run on a separate thread that listens to the terminal output and sends them byte by byte */
void __cdecl outputListener(LPVOID term)
{
	/* TODO: Don't use a fixed buffer maybe? */
	const DWORD BUFF_SIZE { 8192 };
	char szBuffer[BUFF_SIZE]{};
	DWORD dwBytesWritten{};
	DWORD dwBytesRead{};
	BOOL fRead{ FALSE };
	VT100* vt100_term = (VT100*)term;
	do
	{
		/* Read from the pipe */
		fRead = ReadFile(vt100_term->fromProgram, szBuffer, BUFF_SIZE, &dwBytesRead, NULL);
		/* Send it to VT100 parser */
		for (int i = 0; i < dwBytesRead; i++)
		{
			vt100_term->VT100Take(szBuffer[i]);
		}

	} while (fRead && dwBytesRead >= 0);
}

