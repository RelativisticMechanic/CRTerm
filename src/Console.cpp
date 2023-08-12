
#include <Windows.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "Console.h"
#include "Shaders.h"
#include "PNGFont.h"
#include "XTerm256Palette.h"

Console::Console(CRTermConfiguration* cfg, ConsoleFont* fnt)
{
	/* Initialise console dimensions */
	this->console_w = cfg->console_width;
	this->console_h = cfg->console_height;
	this->font_w = fnt->GetXAdvance();
	this->font_h = fnt->GetYAdvance();

	this->cursor_x = 0;
	this->cursor_y = 0;
	
	this->maxlines = cfg->maxlines;
	/* Check the maxlines, if greater than 10000 or less than 500 default to 1000 */
	if (this->maxlines < CONSOLE_MIN_LINES || this->maxlines > CONSOLE_MAX_LINES)
		this->maxlines = CONSOLE_DEFAULT_LINES;

	this->console_resolution_x = this->console_w * this->font_w;
	this->console_resolution_y = this->console_h * this->font_h;

	this->default_fore_color = cfg->default_fore_color;
	this->default_back_color = cfg->default_back_color;
	/* Create the character and attribute buffers */
	this->buffer = (ConsoleChar*)calloc(this->console_w * this->maxlines, sizeof(ConsoleChar));
	this->attrib_buffer = (ConsoleAttrib*)calloc(this->console_w * this->maxlines, sizeof(ConsoleAttrib));

	/* Load the font and the background image */
	this->console_font = fnt;

	if (cfg->crt_background_image != "")
	{
		this->crt_background = GPU_LoadImage(cfg->crt_background_image.c_str());
	}

	this->noise_texture = GPU_LoadImage("ui/noise.png");

	/* Separate render buffer for rendering the console before scaling it. */
	this->render_buffer = GPU_CreateImage(this->console_resolution_x, this->console_resolution_y, GPU_FORMAT_RGBA);
	this->older_frame = GPU_CreateImage(this->console_resolution_x, this->console_resolution_y, GPU_FORMAT_RGBA);
	GPU_LoadTarget(this->render_buffer);
	GPU_LoadTarget(this->older_frame);
	
	/* Load shaders */
	this->common_shader_block = loadShaderProgram(&this->common_shader_id, "shaders/common");
	this->crt_shader_block = loadShaderProgram(&this->crt_shader_id, cfg->shader_path_crt);
	this->text_shader_block = loadShaderProgram(&this->text_shader_id, cfg->shader_path_text);

	/* Load the color scheme */
	for (int i = 0; i < 16; i++)
	{
		this->colors[i] = ConsoleColor(cfg->color_scheme[i].r, cfg->color_scheme[i].g, cfg->color_scheme[i].b);
	}

	/* Load the rest 240 colors from the XTerm palette */
	for (int i = 16; i < 256; i++)
	{
		this->colors[i] = XTermPaletteGetColor(i);
	}

	/* Set CRT effect parameters from configuration */
	this->crt_warp = cfg->crt_warp;
	this->start_line = 0;
	this->last_line = 0;

	/* Clear the console */
	this->Clear();

	this->old_frame_timer = Timer(300);
	/* Set shader parameters */
	/* We set the permanent parameters here instead of every frame. */
	float resolution[2] = { (float)cfg->resolution_x, (float)cfg->resolution_y };
	GPU_ActivateShaderProgram(this->crt_shader_id, &(this->crt_shader_block));
	
	/* Set warp and resolution */
	GPU_SetUniformf(GPU_GetUniformLocation(this->crt_shader_id, "warp"), this->crt_warp);
	GPU_SetUniformfv(GPU_GetUniformLocation(this->crt_shader_id, "resolution"), 2, 1, (float*)&resolution);
	/* Pass the back color to give the glow accent */
	GPU_SetUniformfv(GPU_GetUniformLocation(this->crt_shader_id, "back_color"), 3, 1, this->colors[this->default_back_color].returnArray());
	/* Pass the CRT background image to blend with */
	if (this->crt_background)
	{
		GPU_SetShaderImage(this->crt_background, GPU_GetUniformLocation(this->crt_shader_id, "crt_background"), 1);
	}
	GPU_SetShaderImage(this->noise_texture, GPU_GetUniformLocation(this->crt_shader_id, "noise_texture"), 2);
	
	GPU_DeactivateShaderProgram();
}

void Console::Clear()
{
	for (int y = 0; y < this->console_h; y++)
	{
		for (int x = 0; x < this->console_w; x++)
		{
			int idx = y * this->console_w + x;
			this->PlaceChar(x, y, ' ', this->default_back_color, this->default_back_color);
		}
	}
}

void Console::ClearExt(int fromx, int fromy, int tox, int toy)
{
	last_line = start_line;
	if (fromx < 0)
	{
		fromx = 0;
	}
	if (tox >= this->console_w)
	{
		tox = this->console_w - 1;
	}
	if (fromy < 0)
	{
		fromy = 0;
	}
	if (toy >= this->console_h)
	{
		toy = this->console_h - 1;
	}

	if (fromx > tox)
	{
		std::swap(fromx, tox);
	}

	if (fromy > toy)
	{
		std::swap(fromy, toy);
	}

	for (int i = fromx + fromy * this->console_w; i <= tox + toy * this->console_w; i++)
	{
		this->buffer[start_line * this->console_w + i] = ' ';
		this->attrib_buffer[start_line * this->console_w + i] = CONSTRUCT_ATTRIBUTE(this->default_fore_color, this->default_back_color);
	}
	PREPARE_REDRAW;
}

void Console::LimitCursor()
{
	/* Limit checks for cursor_x and cursor_y, don't go beyond the screen! */
	if (this->cursor_x < 0)
	{
		this->cursor_x = 0;
	}
	if (this->cursor_x >= this->console_w)
	{
		this->cursor_x = this->console_w - 1;
	}
	if (this->cursor_y < 0)
	{
		this->cursor_y = 0;
	}
	if (this->cursor_y >= this->console_h)
	{
		this->cursor_y = this->console_h - 1;
	}
}

void Console::Scroll()
{
	if (start_line < maxlines - console_h)
	{
		start_line++;
	}
	/* If we have exceeded the allocated terminal buffer, it is time to overwrite */
	else
	{
		/* Scroll up by 1 line */
		memcpy(this->buffer, (void*)((uintptr_t)this->buffer + sizeof(ConsoleChar) * 1 * this->console_w), this->console_w * (this->maxlines - 1) * sizeof(ConsoleChar));
		memcpy(this->attrib_buffer, (void*)((uintptr_t)this->attrib_buffer + sizeof(ConsoleAttrib) * this->console_w), this->console_w * (this->maxlines - 1) * sizeof(ConsoleAttrib));
	}
	/* Empty last line */
	for (int i = 0; i < this->console_w; i++)
	{
		this->buffer[(start_line + console_h - 1) * console_w + i] = ' ';
		this->attrib_buffer[(start_line + console_h - 1) * console_w + i] = CONSTRUCT_ATTRIBUTE(this->default_fore_color, this->default_back_color);
	}

	this->cursor_y -= 1;
	this->cursor_x = 0;
	last_line = start_line;
	PREPARE_REDRAW;
}

void Console::HistoryUp()
{
	if (start_line > 0)
	{
		start_line--;
		PREPARE_REDRAW;
	}
}

void Console::HistoryDown()
{
	if (start_line < last_line)
	{
		start_line++;
		PREPARE_REDRAW;
	}
}

void Console::SetSelection(bool selection, int start_x, int start_y, int end_x, int end_y)
{
	this->selected_start_x = start_x;
	this->selected_start_y = start_y;
	this->selected_end_x = end_x;
	this->selected_end_y = end_y;
	this->is_selected = selection;
	PREPARE_REDRAW;
}
void Console::SetCursor(int x, int y)
{
	this->cursor_x = x;
	this->cursor_y = y;
	this->LimitCursor();
	PREPARE_REDRAW;
}

void Console::SetCursorX(int x)
{
	this->SetCursor(x, this->cursor_y);
}

void Console::SetCursorY(int y)
{
	this->SetCursor(this->cursor_x, y);
}

void Console::PlayBell()
{
	PlaySoundA((LPCSTR)SND_ALIAS_SYSTEMASTERISK, NULL, SND_ALIAS_ID | SND_ASYNC);
}

void Console::PutChar(ConsoleChar c) 
{
	Console::PutCharExt(c, this->default_fore_color, this->default_back_color);
}

void Console::PutCharExt(ConsoleChar c, int fore_color, int back_color)
{
	if (c == '\n')
	{
		this->cursor_y += 1;
		this->cursor_x = 0;
	}
	else if (c == '\r')
	{
		this->cursor_x = 0;
	}
	else if (c == '\b')
	{
		if (this->cursor_x <= 0)
		{
			this->cursor_y -= 1;
			this->cursor_x = this->console_w - 1;
		}
		else
		{
			this->cursor_x -= 1;
		}
	}
	else if (c == '\7')
	{
		this->PlayBell();
	}
	else if (c == '\t')
	{
		int n = 8 - (this->cursor_x % 8);
		for (int i = 0; i < n; i++)
		{
			/* Don't exceed beyond screen width */
			this->PutCharExt(' ', fore_color, back_color);
		}
	}
	else
	{
		/* Wrap around */
		if (this->cursor_x >= this->console_w)
		{
			if (this->wrap_around)
			{
				this->cursor_x = 0;
				this->cursor_y += 1;
			}
			else
			{
				while (!(this->cursor_x < this->console_w))
					this->cursor_x -= 1;
			}
		}

		this->PlaceChar(this->cursor_x, this->cursor_y, c, fore_color, back_color);
		this->cursor_x++;
	}

	if (this->cursor_y >= this->console_h)
	{
		while(!(this->cursor_y < this->console_h))
			this->Scroll();
	}
	PREPARE_REDRAW;
}

void Console::PlaceChar(int x, int y, ConsoleChar c, int fore_color, int back_color)
{
	start_line = last_line;
	if (x < 0 || y < 0 || x >= this->console_w || y >= this->console_h) return;
	this->buffer[start_line * this->console_w + y * this->console_w + x] = c;

	ConsoleAttrib attrib = CONSTRUCT_ATTRIBUTE(fore_color, back_color);

	this->attrib_buffer[(start_line + y) * this->console_w + x] = attrib;
	PREPARE_REDRAW;
}

void Console::Puts(std::string s)
{
	for (int i = 0; i < s.length(); i++)
	{
		this->PutChar(s[i]);
	}
}

void Console::ShowCursor()
{
	// STUB
}

void Console::HideCursor()
{
	// STUB
}

void Console::EnableWrapAround()
{
	this->wrap_around = true;
}

void Console::DisableWrapAround()
{
	this->wrap_around = false;
}

ConsoleChar Console::ReadChar(int x, int y)
{
	if (x < 0 || y < 0 || x >= this->console_w || y >= this->console_h)
		return ' ';
	return this->buffer[(this->start_line + y) * this->console_w + x];
}

/* Redraws into the internal render_buffer - This function is costly, so its only called when redraw_console is true. */
void Console::Redraw(void)
{
	GPU_Clear(this->render_buffer->target);
	/* Enable text shader */
	GPU_ActivateShaderProgram(this->text_shader_id, &this->text_shader_block);

	/* Set text alpha to 1.0 */
	GPU_SetUniformf(GPU_GetUniformLocation(this->text_shader_id, "alpha"), 1.0);
	for (int y = 0; y < this->console_h; y++)
	{
		for (int x = 0; x < this->console_w; x++)
		{
			int xcur = (x * this->font_w);
			int ycur = (y * this->font_h);
			ConsoleChar c = this->buffer[(start_line + y) * this->console_w + x];
			ConsoleAttrib attrib = this->attrib_buffer[(start_line + y) * this->console_w + x];
			/* Lower 4 bits = text color, higher 4 bits = background color */
			int text_color_idx = attrib & 0xFF;
			int back_color_idx = (attrib & 0xFF00) >> 8;

			ConsoleColor text_color = this->colors[text_color_idx];
			ConsoleColor back_color = this->colors[back_color_idx];

			/* If its backcolor, don't draw, ignore if we are in 8-bit color mode.
			We already pass back color to the shader */

			if (back_color_idx != this->default_back_color)
			{
				/* If it is not the default back color draw a rect with the backcolor */
				GPU_SetUniformf(GPU_GetUniformLocation(this->text_shader_id, "alpha"), 0.5);
				GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, back_color.returnArray());
				GPU_RectangleFilled(this->render_buffer->target, xcur, ycur, xcur + this->font_w, ycur + this->font_h, SDL_Color{ 255, 255, 255, 255 });
				GPU_SetUniformf(GPU_GetUniformLocation(this->text_shader_id, "alpha"), 1.0);
			}
			GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, text_color.returnArray());
			GPU_Blit(this->console_font->GetGlyph(c), NULL, this->render_buffer->target, xcur + font_w / 2, ycur + font_h / 2);
		}
	}
	/* Draw the scroll bar if we are scrolling i.e. last_line != start_line and the user has scrolled up */
	if (start_line != last_line && this->last_line != 0)
	{
		GPU_Rect scrollbar;
		scrollbar.h = (((float)this->console_h) / ((float)this->last_line + this->console_h)) * (this->render_buffer->h);
		scrollbar.y = (((float)this->start_line) / (float)(this->last_line)) * this->render_buffer->h;
		scrollbar.w = 8;
		scrollbar.x = this->render_buffer->w - 8;
		GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, this->colors[this->default_fore_color].returnArray());
		/* Set scroll bar alpha to 0.8 */
		GPU_SetUniformf(GPU_GetUniformLocation(this->text_shader_id, "alpha"), 0.8);
		GPU_RectangleFilled(this->render_buffer->target, scrollbar.x, scrollbar.y, scrollbar.x + scrollbar.w, scrollbar.y + scrollbar.h, SDL_Color{ 255, 255, 255, 255 });
	}

	/* Draw the selection rectangle if the user has selected some text, this bool passed from VT100 class */
	if (this->is_selected)
	{
		GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, this->colors[this->default_fore_color].returnArray());
		/* Set selected text alpha to 0.4 */
		GPU_SetUniformf(GPU_GetUniformLocation(this->text_shader_id, "alpha"), 0.2);
		for (int i = this->selected_start_x + this->selected_start_y * this->console_w; i < this->selected_end_x + this->selected_end_y * this->console_w; i++)
		{

			int y = i / this->console_w;
			int x = i % this->console_w;
			GPU_RectangleFilled(this->render_buffer->target, x * this->font_w, y * this->font_h, (x + 1) * this->font_w, (y + 1) * this->font_h, SDL_Color{ 255, 255, 255, 255 });
		}
	}

	/* Draw the cursor, if we are not scrolling */
	if (this->start_line == this->last_line)
	{
		GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, this->colors[this->default_fore_color].returnArray());
		/* Cursor should be partially see-through */
		GPU_SetUniformf(GPU_GetUniformLocation(this->text_shader_id, "alpha"), 0.85);
		GPU_RectangleFilled(this->render_buffer->target, cursor_x * font_w, cursor_y * font_h, (cursor_x + 1) * font_w, (cursor_y + 1) * font_h, SDL_Color{ 255, 255, 255, 255 });
	}
}
/* The Console Render function, this is what makes the magic happen :) */
void Console::Render(GPU_Target* t, int xloc, int yloc, float scale)
{

	if (old_frame_timer.Elapsed())
	{
		GPU_ActivateShaderProgram(this->common_shader_id, &this->common_shader_block);
		GPU_Clear(this->older_frame->target);
		GPU_Blit(this->render_buffer, NULL, this->older_frame->target, this->older_frame->w / 2, this->older_frame->h / 2);
		this->last_burn_in_time = SDL_GetTicks64();
	}

	/* If pending redraw, draw it. */
	if (this->redraw_console)
	{
		this->Redraw();
		this->redraw_console = false;
	}
	/* 
		We have now rendered the terminal to the local render buffer,
		now we pass it through the CRT shader and scale it up.
		Now apply the CRT effect shader
		The CRT effect shader applies CRT warp effect, CRT phosphor glow scanline effect, and noise.
	*/
	GPU_ActivateShaderProgram(this->crt_shader_id, &this->crt_shader_block);

	/* Set time */
	GPU_SetUniformf(GPU_GetUniformLocation(this->crt_shader_id, "time"), ((float)SDL_GetTicks64()) / 1000.0);
	/* Set Burn In Effect Parameters */
	GPU_SetShaderImage(this->older_frame, GPU_GetUniformLocation(this->crt_shader_id, "older_frame"), 3);
	/* These are sent in ms for accuracy */
	GPU_SetUniformf(GPU_GetUniformLocation(this->crt_shader_id, "current_frame_time"), SDL_GetTicks64());
	GPU_SetUniformf(GPU_GetUniformLocation(this->crt_shader_id, "older_frame_time"), this->last_burn_in_time);
	/* Now blit to screen! */
	GPU_BlitScale(this->render_buffer, NULL, t, xloc + (int)(this->render_buffer->w / 2) * scale, yloc + (int)(this->render_buffer->h / 2) * scale, scale, scale);
	GPU_DeactivateShaderProgram();
}