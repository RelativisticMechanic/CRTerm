
#include <Windows.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "Console.h"
#include "Shaders.h"
#include "CustomTitleBar.h"

Console::Console(CRTermConfiguration* cfg)
{
	/* Initialise console dimensions */
	this->console_w = cfg->console_width;
	this->console_h = cfg->console_height;
	this->font_w = cfg->font_width;
	this->font_h = cfg->font_height;

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
	this->buffer = (unsigned char*)calloc(this->console_w * this->maxlines, sizeof(char));
	this->attrib_buffer = (unsigned char*)calloc(this->console_w * this->maxlines, sizeof(char));

	/* Load the font and the background image */
	this->console_font = GPU_LoadImage(cfg->bitmap_font_file.c_str());
	this->crt_background = GPU_LoadImage(cfg->crt_background_image.c_str());

	/* Load the bell sound */
	std::ifstream bell_file;
	bell_file.open(cfg->bell_sound);

	std::stringstream str_bell_stream;
	str_bell_stream << bell_file.rdbuf();
	this->bell_wave_file = str_bell_stream.str();

	/* 
	Now create 256 GPU_Targets each containing
	the character data from the file.
	*/
	int chars_per_row = this->console_font->w / this->font_w;
	for (int i = 0; i < 256; i++)
	{
		GPU_Rect r; 
		r.x = (i % chars_per_row) * this->font_w;
		r.y = (i / chars_per_row) * this->font_h;
		r.w = this->font_w;
		r.h = this->font_h;
		this->char_blocks[i] = GPU_CreateImage(this->font_w, this->font_h, GPU_FORMAT_RGBA);
		GPU_Target* tg = GPU_LoadTarget(this->char_blocks[i]);
		if (tg != this->char_blocks[i]->target || tg == NULL)
		{
			std::cout << "Warning: GPU_LoadTarget Failed!" << std::endl;
		}
		GPU_Blit(this->console_font, &r, tg, this->font_w / 2.0, this->font_h / 2.0);
	}

	/* Separate render buffer for rendering the console before scaling it. */
	this->render_buffer = GPU_CreateImage(this->console_resolution_x, this->console_resolution_y, GPU_FORMAT_RGBA);
	GPU_LoadTarget(this->render_buffer);

	/* Load shaders */
	this->crt_shader_block = loadShaderProgram(&this->crt_shader_id, cfg->shader_path_crt);
	this->text_shader_block = loadShaderProgram(&this->text_shader_id, cfg->shader_path_text);

	/* Cursor clock for blinking */
	this->blink_interval = cfg->blink_interval;
	this->cursor_clock = SDL_GetTicks64();
	this->prev_time = SDL_GetTicks64();
	this->show_cursor = true;

	/* Load the color scheme */
	for (int i = 0; i < 16; i++)
	{
		this->color_scheme[i] = ConsoleColor(cfg->color_scheme[i].r, cfg->color_scheme[i].g, cfg->color_scheme[i].b);
	}

	/* Set CRT effect parameters from configuration */
	this->crt_warp = cfg->crt_warp;

	this->start_line = 0;
	this->last_line = 0;
	/* Clear the console */
	this->Clear();
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
		memcpy(this->buffer, (void*)((uintptr_t)this->buffer + sizeof(char) * 1 * this->console_w), this->console_w * (this->maxlines - 1));
		memcpy(this->attrib_buffer, (void*)((uintptr_t)this->attrib_buffer + sizeof(char) * this->console_w), this->console_w * (this->maxlines - 1));
	}
	/* Empty last line */
	memset((void*)((uintptr_t)this->buffer + sizeof(char) * (this->start_line + this->console_h - 1) * this->console_w), ' ', this->console_w);
	memset((void*)((uintptr_t)this->attrib_buffer + sizeof(char) * (this->start_line + this->console_h - 1) * this->console_w), CONSTRUCT_ATTRIBUTE(this->default_fore_color, this->default_back_color), this->console_w);
	this->cursor_y -= 1;
	this->cursor_x = 0;
	last_line = start_line;
}

void Console::HistoryUp()
{
	if(start_line > 0)
		start_line--;
}

void Console::HistoryDown()
{
	if (start_line < last_line)
		start_line++;
}

void Console::SetSelection(bool selection, int start_x, int start_y, int end_x, int end_y)
{
	this->selected_start_x = start_x;
	this->selected_start_y = start_y;
	this->selected_end_x = end_x;
	this->selected_end_y = end_y;
	this->is_selected = selection;
}
void Console::SetCursor(int x, int y)
{
	this->cursor_x = x;
	this->cursor_y = y;
	this->LimitCursor();
}

void Console::SetCursorX(int x)
{
	this->cursor_x = x;
	this->LimitCursor();
}

void Console::SetCursorY(int y)
{
	this->cursor_y = y;
	this->LimitCursor();
}

void Console::PlayBell()
{
	PlaySoundA(this->bell_wave_file.c_str(), NULL, SND_MEMORY | SND_ASYNC);
}

void Console::PutChar(unsigned char c) 
{
	Console::PutCharExt(c, this->default_fore_color, this->default_back_color);
}

void Console::PutCharExt(unsigned char c, int fore_color, int back_color)
{
	uint8_t chr = (uint8_t)c;
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
			this->PutCharExt(' ', fore_color, back_color);
		}
	}
	else
	{
		this->PlaceChar(this->cursor_x, this->cursor_y, chr, fore_color, back_color);
		/* Set cursor shadow, this is what creates the cursor "burn in" effect */
		this->cursor_shadow_width = this->font_w * 2;
		this->cursor_x += 1;
	}

	if (this->cursor_x >= this->console_w && this->cursor_y < this->console_h - 1)
	{
		this->cursor_x = 0;
		this->cursor_y += 1;
	}

	if (this->cursor_y >= this->console_h)
	{
		while(!(this->cursor_y < this->console_h))
			this->Scroll();
	}

	this->LimitCursor();

}

void Console::PlaceChar(int x, int y, unsigned char c, int fore_color, int back_color)
{
	start_line = last_line;
	if (x < 0 || y < 0 || x >= this->console_w || y >= this->console_h) return;
	this->buffer[start_line * this->console_w + y * this->console_w + x] = c;
	this->attrib_buffer[start_line * this->console_w + y * this->console_w + x] = CONSTRUCT_ATTRIBUTE(fore_color, back_color);
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
	this->show_cursor = true;
	this->cursor_clock = SDL_GetTicks64();
}

void Console::HideCursor()
{
	this->show_cursor = false;
	this->cursor_clock = SDL_GetTicks64();
}

unsigned char Console::ReadChar(int x, int y)
{
	if (x < 0 || y < 0 || x >= this->console_w || y >= this->console_h)
		return 0;
	return this->buffer[this->start_line * this->console_w + y * this->console_w + x];
}

/* The Console Render function, this is what makes the magic happen :) */
void Console::Render(GPU_Target* t, int xloc, int yloc, float scale)
{
	GPU_Clear(this->render_buffer->target);
	uint64_t time = SDL_GetTicks64();
	this->delta_time = time - this->prev_time;
	/* Enable text shader */
	GPU_ActivateShaderProgram(this->text_shader_id, &this->text_shader_block);
	/* Blink Cursor */
	if (time - this->cursor_clock > this->blink_interval)
	{
		this->show_cursor = !this->show_cursor;
		this->cursor_clock = time;
	}
	/* Set text alpha to 1.0 */
	GPU_SetUniformf(GPU_GetUniformLocation(this->text_shader_id, "alpha"), 1.0);
	for (int y = 0; y < this->console_h; y++)
	{
		for (int x = 0; x < this->console_w; x++)
		{
			int xcur = (x * this->font_w);
			int ycur = (y * this->font_h);
			unsigned char c = this->buffer[start_line * this->console_w + y * this->console_w + x];
			unsigned char attrib = this->attrib_buffer[start_line * this->console_w + y * this->console_w + x];
			/* Lower 4 bits = text color, higher 4 bits = background color */
			int text_color = attrib & 0x0F;
			int back_color = (attrib & 0xF0) >> 4;
			/* If its backcolor, don't draw, we already pass back color to the shader */
			if (back_color != this->default_back_color)
			{
				/* If it is not back_color draw a block (ASCII 219) with the backcolor */
				GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, this->color_scheme[back_color].returnArray());
				GPU_Blit(this->char_blocks[219], NULL, this->render_buffer->target, xcur + font_w / 2, ycur + font_h / 2);
			}
			GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, this->color_scheme[text_color].returnArray());
			GPU_Blit(this->char_blocks[c], NULL, this->render_buffer->target, xcur + font_w / 2, ycur + font_h / 2);
		}
	}

	/* Draw the scroll bar if we are scrolling i.e. last_line != start_line and the user has scrolled up */
	if (start_line != last_line)
	{
		GPU_Rect scrollbar;
		scrollbar.h = (((float)this->console_h) / ((float)this->last_line + this->console_h)) * (this->render_buffer->h);
		scrollbar.y = scrollbar.h * 0.5 * ((float)this->start_line / (float)(this->last_line + 1.0));
		scrollbar.w = 8;
		scrollbar.x = this->render_buffer->w - 8;
		GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, this->color_scheme[this->default_fore_color].returnArray());
		/* Set scroll bar alpha to 0.8 */
		GPU_SetUniformf(GPU_GetUniformLocation(this->text_shader_id, "alpha"), 0.8);
		GPU_RectangleFilled(this->render_buffer->target, scrollbar.x, scrollbar.y, scrollbar.x + scrollbar.w, scrollbar.y + scrollbar.h, SDL_Color{ 255, 255, 255, 255 });
	}

	/* Draw the selection rectangle if the user has selected some text, this bool passed from VT100 class */
	if (this->is_selected)
	{
		GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, this->color_scheme[this->default_fore_color].returnArray());
		/* Set selected text alpha to 0.4 */
		GPU_SetUniformf(GPU_GetUniformLocation(this->text_shader_id, "alpha"), 0.4);
		for (int i = this->selected_start_x + this->selected_start_y * this->console_w; i < this->selected_end_x + this->selected_end_y * this->console_w; i++)
		{
			
			int y = i / this->console_w;
			int x = i % this->console_w;
			GPU_RectangleFilled(this->render_buffer->target, x * this->font_w, y * this->font_h, (x + 1) * this->font_w, (y + 1) * this->font_h, SDL_Color{ 255, 255, 255, 255 });
		}
	}

	/* Draw the cursor, if we are not scrolling */
	if (this->show_cursor && this->start_line == this->last_line)
	{
		GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, this->color_scheme[this->default_fore_color].returnArray());
		GPU_Blit(this->char_blocks[219], NULL, this->render_buffer->target, this->cursor_x * this->font_w + font_w / 2, this->cursor_y * this->font_h + font_h / 2);

		/* Draw cursor shadow */
		if (this->cursor_shadow_width > 0.0)
		{
			GPU_SetUniformf(GPU_GetUniformLocation(this->text_shader_id, "alpha"), 0.5);
			GPU_RectangleFilled(this->render_buffer->target, this->cursor_x * this->font_w - (this->cursor_shadow_width / 2.0), this->cursor_y * this->font_h, this->cursor_x * this->font_w + (this->cursor_shadow_width / 2.0), this->cursor_y * this->font_h + this->font_h, SDL_Color{255, 255, 255, 255});
			GPU_SetUniformf(GPU_GetUniformLocation(this->text_shader_id, "alpha"), 1.0);
			/* Reduce size of cursor shadow with time. */
			this->cursor_shadow_width -= (10*this->delta_time / 100.0);
		}
	}

	/* 
		We have now rendered the terminal to the local render buffer,
		now we pass it through the CRT shader and scale it up.
	*/
	GPU_ActivateShaderProgram(this->crt_shader_id, &this->crt_shader_block);
	float resolution[2] = { (float)t->w, (float)t->h };
	/* Set shader parameters */
	GPU_SetUniformf(GPU_GetUniformLocation(this->crt_shader_id, "warp"), this->crt_warp);
	GPU_SetUniformf(GPU_GetUniformLocation(this->crt_shader_id, "time"), ((float)SDL_GetTicks64())/ 1000.0);
	GPU_SetUniformfv(GPU_GetUniformLocation(this->crt_shader_id, "resolution"), 2, 1, (float*)&resolution);
	/* Pass the back color to give the glow accent */
	GPU_SetUniformfv(GPU_GetUniformLocation(this->crt_shader_id, "back_color"), 3, 1, this->color_scheme[this->default_back_color].returnArray());
	/* Pass the CRT background image to blend with */
	GPU_SetShaderImage(this->crt_background, GPU_GetUniformLocation(this->crt_shader_id, "crt_background"), 1);
	/* Now blit to screen! */
	GPU_BlitScale(this->render_buffer, NULL, t, xloc + (int)(this->render_buffer->w / 2) * scale, yloc + (int)(this->render_buffer->h / 2) * scale, scale, scale);
	GPU_DeactivateShaderProgram();
	/*
	Now apply the CRT effect shader
	The CRT effect shader applies CRT warp effect, CRT phosphor glow scanline effect, and noise.

	/* This constant is used for the burn in effect clock */
	this->prev_time = time;
}