#include <iostream>
#include <conio.h>
#include <Windows.h>
#include "CRTerm.h"
#include "VT100.h"
#include "ConPTY.h"
#include "Win32ClipBoard.h"

void __cdecl outputListener(LPVOID term);

VT100::VT100(CRTermConfiguration* cfg, ConsoleFont* fnt, GPU_Target* render_target)
{
	con = new Console(cfg, fnt);
	this->render_target = render_target;
	this->fg = con->default_fore_color;
	this->bg = con->default_back_color;
	this->bracketed_mode = true;
	this->CTRL_down = false;
	this->is_selected = false;
	this->parser_state = VTSTATE_NORMAL;
	this->console_window = GetActiveWindow();
	this->sdl_window = SDL_GetWindowFromID(render_target->context->windowID);
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

	/* Intro screen */
	VT100Puts(CRTERM_VERSION_STRING);
	VT100Puts("\n");
	VT100Puts(CRTERM_CREDIT_STRING);
	VT100Puts("Terminal initialized.\n");
	VT100Puts("Loading shell...");
	HRESULT ok = SpawnProcessinConsole((wchar_t*)process_name.c_str(), hPC, &cmd_process);
	if (ok != S_OK)
	{
		VT100Puts("Failed to initialize provided shell application.\n");
	}
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
		}
		else if (c == '>')
		{
			/* ESC >: Exit alternate keypad mode */
			this->alternate_keypad_mode = false;
			parser_state = VTSTATE_NORMAL;
		}
		else if (c == '=')
		{
			/* ESC =: Enter alternate keypad mode */
			this->alternate_keypad_mode = true;
			parser_state = VTSTATE_NORMAL;
		}
		else
		{
			std::cout << "Unexpected identifier after VT_ESCAPE: " << c << std::endl;
			parser_state = VTSTATE_NORMAL;
		}
		break;
	case VTSTATE_ATTR:
	case VTSTATE_PRIVATE:
	case VTSTATE_CONTROL_STRING:
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
				if (parser_state == VTSTATE_ATTR)
					parser_state = VTSTATE_ENDVAL;
				else if (parser_state == VTSTATE_PRIVATE)
					parser_state = VTSTATE_PRIVATE_ENDVAL;
				else if (parser_state == VTSTATE_CONTROL_STRING)
					parser_state = VTSTATE_CONTROL_ENDVAL;
			}
		}
		break;
	case VTSTATE_TAKE_STRING:
		if (c == '\x1B' || c == '\x7')
		{
			if (c == '\x1B')
				parser_state = VTSTATE_END_STRING;
			else
				parser_state = VTSTATE_NORMAL;

			/* Set the window title to 0 string */
			std::string window_title = "Gautam's CRTerm - " + title;
			SDL_SetWindowTitle(this->sdl_window, window_title.c_str());
		}
		else
		{
			if (control_string_idx == 0)
			{
				title += c;
			}
		}
		break;

	case VTSTATE_END_STRING:
		parser_state = VTSTATE_NORMAL;
		if (c != '\\')
		{
			std::cerr << "Warning: Expected an ESC \\ at the end of ESC [ cmd; <string> ESC \\ but got '" << c << "'" << std::endl;
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
				if (stack_ptr <= 1 && argument_stack[stack_ptr].empty)
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
				/* Special graphics attributes */
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
						/* Make bold if 1 */
						if (attr == 1)
						{
							/* 
							NOTE for v0.3.0, 
							Since this seems to mess up neofetch if we make bright, and we have not implemented bold yet, we simply ignore.
							
							this->fg |= 8;
							if(this->bg != con->default_back_color)
								this->bg |= 8;
							*/
						}
						/* Swap bg and fg */
						else if (attr == 7 || attr == 27)
						{
							//std::swap(this->bg, this->fg);
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
						else if (attr == 39)
						{
							this->fg = con->default_fore_color;
						}
						else if (attr == 49)
						{
							this->bg = con->default_back_color;
						}
						/* Beginning of ISO 8613 sequences */
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
										this->fg = argument_stack[i].value + 16;
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
										/* 16 is added to tell PlaceChar to know this is 8-bit color */
										this->bg = argument_stack[i].value + 16;
									}
								}
							}
						}
						/* 90-97: Set bright FG */
						else if (attr >= 90 && attr <= 97)
						{
							this->fg = (attr - 90);
							this->fg |= 8;
						}
						/* 100 - 107 set bright BG */
						else if (attr >= 100 && attr <= 107)
						{
							this->bg = (attr - 90);
							if(this->bg != this->con->default_back_color)
								this->bg |= 8;
						}
						/* Normal intensity */
						else if (attr == 22)
						{
							this->fg &= 7;
							this->bg &= 7;
						}
						else
						{
							std::cout << "Unhandled SGR attribute: " << attr << std::endl;
						}
					}
				}
				break;
			case 'n':
				/* CSI 6n Report terminal parameters */
				if (argument_stack[0].value == 6)
				{
					std::string term_info = "\x1B[" + std::to_string(con->cursor_y + 1) + ';' + std::to_string(con->cursor_x + 1) + 'R';
					WriteFile(this->toProgram, term_info.c_str(), term_info.length(), NULL, NULL);
				}
				/* ESC 0n - Report status terminal status */
				else if (argument_stack[0].value == 5)
				{
					// Report OK.
					std::string status_ok = "\x1B[0n";
					WriteFile(this->toProgram, status_ok.c_str(), status_ok.length(), NULL, NULL);
				}
				break;
			/* ESC [0c or ESC c: Report terminal version. We report as VT100 */
			case 'c':
				WriteFile(this->toProgram, "\x1B[?;0c", strlen("\x1B[?;0c"), NULL, NULL);
				break;
			case 'X':
				/* Erase n chars to the right */
				if (argument_stack[0].empty)
				{
					con->PlaceChar(con->cursor_x, con->cursor_y, ' ', con->default_fore_color, con->default_back_color);
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
			else if (this->argument_stack[0].value == 2004)
			{
				this->bracketed_mode = true;
			}
			/* CSI ? 2 h DEC Turn ON disabled keyboard input */
			else if (this->argument_stack[0].value == 2)
			{
				/* TODO: This causes some UTF-8 issues for now */
				/* this->keyboard_disabled = true; */
			}
			/* CSI ? 7 h DEC Turn ON wraparound */
			else if (this->argument_stack[0].value == 7)
			{
				con->EnableWrapAround();
			}
			/* CSI ? 12 h DEC Turn ON Send-Receive Mode */
			else if (this->argument_stack[0].value == 12)
			{
				// IGNORE.
			}
			/* CSI ? 1004 h - DEC Turn ON reporting focus change */
			else if (this->argument_stack[0].value == 1004)
			{
				this->report_focus_change = true;
			}
			else
			{
				std::cout << "Unimplemented ON: " << this->argument_stack[0].value << std::endl;
			}
			break;
		case 'l':
			/* CSI ? 25 l Hide cursor */
			if (this->argument_stack[0].value == 25)
			{
				con->ShowCursor();
			}
			/* CST ? 2004 l Turn OFF bracketed paste */
			else if (this->argument_stack[0].value == 2004)
			{
				this->bracketed_mode = false;
			}
			/* CSI ? 2 l DEC Turn OFF Keyboard Disable */
			else if (this->argument_stack[0].value == 2)
			{
				this->keyboard_disabled = false;
			}
			/* CSI ? 7 l DEC Turn OFF Wrap Around */
			else if (this->argument_stack[0].value == 7)
			{
				con->DisableWrapAround();
			}
			/* CSI ? 12 l DEC Turn OFF Send-Receive Mode */
			else if (this->argument_stack[0].value == 12)
			{
				// IGNORE.
			}
			/* CSI ? 1004 h - DEC Turn OFF reporting focus change */
			else if (this->argument_stack[0].value == 1004)
			{
				this->report_focus_change = false;
			}
			else
			{
				std::cout << "Unimplemented OFF: " << this->argument_stack[0].value << std::endl;
			}
			break;
		case '$':
			/* CSI ? ARGS $p - Unimplemented */
			parser_state = VTSTATE_IGNORE_NEXT;
			break;
		default:
			std::cout << "Unimplemented private sequence: " << c << std::endl;
			break;
		}
		if (parser_state != VTSTATE_IGNORE_NEXT)
			parser_state = VTSTATE_NORMAL;
	}

	if (parser_state == VTSTATE_CONTROL_ENDVAL)
	{
		control_string_idx = argument_stack[0].value;
		switch (control_string_idx)
		{
			/* ESC ] 0; <title> Set Title */
		case 0:
			title = "";
			parser_state = VTSTATE_TAKE_STRING;
			break;
			/* ESC ] 8; <string> Hyperlink */
		case 8:
		default:
			std::cerr << "Unimplemented control string: " << argument_stack[0].value << std::endl;
			parser_state = VTSTATE_TAKE_STRING;
			break;
		}

	}
}

void VT100::VT100Puts(std::string s)
{
	for (int i = 0; i < s.length(); i++)
	{
		this->VT100Putc(s[i]);
	}
}
void VT100::VT100Putc(unsigned char c)
{
	/* Are we in midst of a UTF-8 character? Recall that UTF-8 characters can usually be made of 1,2,3 or 4 bytes */
	if (!utf8_bytes_left)
	{
		int unicode_check = (c & 0b11110000) >> 4;
		/* Is 'c' unicode? */
		switch (unicode_check)
		{
		case 0b1100:
			/* UTF-8 1 byte left */
			utf8_bytes_left = 1;
			utf8_char = (c & 0b00011111) << 6;
			break;
		case 0b1110:
			/* UTF-8 2 bytes left */
			utf8_bytes_left = 2;
			utf8_char = (c & 0b00001111) << 12;
			break;
		case 0b1111:
			/* UTF-8 3 bytes left */
			utf8_bytes_left = 3;
			utf8_char = (c << 0b0000011) << 18;
			break;
		default:
			/* Not UTF-8, print as is */
			con->PutCharExt(c, this->fg, this->bg);
			break;
		}
	}
	/* Yes, we are, decrement. */
	else
	{
		/* Skip this byte as it is unicode */
		utf8_bytes_left -= 1;
		/* Store it */
		switch (utf8_bytes_left)
		{
		case 0:
			utf8_char |= (c & 0b111111);
			break;
		case 1:
			utf8_char |= (c & 0b111111) << 6;
			break;
		case 2:
			utf8_char |= (c & 0b111111) << 12;
			break;
		default:
			break;
		}
		/* Draw the UTF-8 Char */
		if (utf8_bytes_left == 0)
		{
			con->PutCharExt(utf8_char, this->fg, this->bg);
		}
	}
}

void VT100::VT100HandleEvent(SDL_Event ev)
{
	switch (ev.type)
	{
	case SDL_TEXTINPUT:
		if (!keyboard_disabled)
		{
			/* Send text input as is, SDLs text input is ASCII compliant */
			WriteFile(this->toProgram, &(ev.text.text[0]), 1, NULL, NULL);
		}
		break;
	case SDL_KEYDOWN:
		if (keyboard_disabled)
			break;

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

		/* If CTRL is down... send the relevant sequence. */
		if (this->CTRL_down)
		{
			if (control_key_map.find((int)ev.key.keysym.sym) != control_key_map.end())
			{
				std::string control_sequence = control_key_map[(int)ev.key.keysym.sym];
				WriteFile(this->toProgram, control_sequence.c_str(), control_sequence.length(), NULL, NULL);
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
	case SDL_MOUSEWHEEL:
		if (ev.wheel.y > 0)
		{
			this->con->HistoryUp();
		}
		else
		{
			this->con->HistoryDown();
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
	case SDL_WINDOWEVENT:
		switch (ev.window.event)
		{
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			if (this->report_focus_change)
			{
				WriteFile(this->toProgram, "\x1B[I", 3, NULL, NULL);
			}
			break;
		case SDL_WINDOWEVENT_FOCUS_LOST:
			if (this->report_focus_change)
			{
				WriteFile(this->toProgram, "\x1B[O", 3, NULL, NULL);
			}
			break;
		default:
			break;
		}
		break;
	case SDL_QUIT:
		VT100Shutdown();
		break;
	default:
		break;
	}
}
/* Send characters to the terminal */
void VT100::VT100Send(std::string sequence)
{
	WriteFile(this->toProgram, sequence.c_str(), sequence.length(), NULL, NULL);
}

void VT100::VT100Render(void)
{
	/* Draw overlay above the selected text if it is selected or dragging */
	if (is_selected || is_dragging)
	{
		orientSelectedCoords();
		this->con->SetSelection(true, selected_start_x, selected_start_y, selected_end_x, selected_end_y);
	}
	else
	{
		this->con->SetSelection(false);
	}
	con->Render(this->render_target, this->screen_offsetx, this->screen_offsety, this->font_scale);
	GPU_DeactivateShaderProgram();
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
		for (int i = this->selected_start_x + this->selected_start_y * this->con->console_w; i <= this->selected_end_x + this->selected_end_y * this->con->console_w; i++)
		{
			int y = i / this->con->console_w;
			int x = i % this->con->console_w;

			/* Decode UTF-32 to UTF-8 */

			uint32_t utf32 = this->con->ReadChar(x, y);
			std::cout << "COPYING CHAR (UTF-32)" << (int)utf32 << std::endl;
			if (utf32 < 0x80) 
			{
				result += (char)utf32;
			}
			else if (utf32 < 0x800) 
			{   // 00000yyy yyxxxxxx
				result += (char)(0b11000000 | (utf32 >> 6));
				result += (char)(0b10000000 | (utf32 & 0x3f));
			}
			else if (utf32 < 0x10000) 
			{  // zzzzyyyy yyxxxxxx
				result += (char)(0b11100000 | (utf32 >> 12));			// 1110zzz
				result += (char)(0b10000000 | ((utf32 >> 6) & 0x3f));	// 10yyyyy
				result += (char)(0b10000000 | (utf32 & 0x3f));			// 10xxxxx
			}
			else if (utf32 < 0x200000) 
			{ // 000uuuuu zzzzyyyy yyxxxxxx
				result += (char)(0b11110000 | (utf32 >> 18));			// 11110uuu
				result += (char)(0b10000000 | ((utf32 >> 12) & 0x3f));	// 10uuzzzz
				result += (char)(0b10000000 | ((utf32 >> 6) & 0x3f));	// 10yyyyyy
				result += (char)(0b10000000 | (utf32 & 0x3f));			// 10xxxxxx
			}
		}
		std::cout << "-----" << std::endl;
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
		/* Check if process is still active */
		DWORD exitcode;
		if (GetExitCodeProcess(vt100_term->cmd_process.hProcess, &exitcode))
		{
			if (exitcode != STILL_ACTIVE)
			{
				vt100_term->con->Puts("Process exited with exit code: " + std::to_string((int)exitcode) + std::string("\n"));
				vt100_term->con->Puts("You may exit the terminal.");
				break;
			}
		}

		/* Read from the pipe */
		fRead = ReadFile(vt100_term->fromProgram, szBuffer, BUFF_SIZE, &dwBytesRead, NULL);
		/* Send it to VT100 parser */
		for (int i = 0; i < dwBytesRead; i++)
		{
			vt100_term->VT100Take(szBuffer[i]);
		}

	} while (fRead && dwBytesRead >= 0);
}

