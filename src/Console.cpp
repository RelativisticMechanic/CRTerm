#include "Console.h"
#include "Shaders.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <sstream>

Console::Console(CRTermConfiguration* cfg)
{
	// Initialise console dimensions
	this->console_w = cfg->console_width;
	this->console_h = cfg->console_height;
	this->font_w = cfg->font_width;
	this->font_h = cfg->font_height;

	this->cursor_x = 0;
	this->cursor_y = 0;
	
	this->console_resolution_x = this->console_w * this->font_w;
	this->console_resolution_y = this->console_h * this->font_h;

	this->default_fore_color = cfg->default_fore_color;
	this->default_back_color = cfg->default_back_color;
	// Create the character and attribute buffers
	this->buffer = (unsigned char*)calloc(this->console_w * this->console_h, sizeof(char));
	this->attrib_buffer = (unsigned char*)calloc(this->console_w * this->console_h, sizeof(char));

	// Load the font and the background image
	this->console_font = GPU_LoadImage(cfg->bitmap_font_file.c_str());
	this->crt_background = GPU_LoadImage(cfg->crt_background_image.c_str());

	// Load the bell sound
	std::ifstream bell_file;
	bell_file.open(cfg->bell_sound); //open the input file

	std::stringstream str_bell_stream;
	str_bell_stream << bell_file.rdbuf(); //read the file
	this->bell_wave_file = str_bell_stream.str();

	// Now create 256 targets each containing 
	// the data from the font file
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
		// The target is the character image 
		GPU_Blit(this->console_font, &r, tg, this->font_w / 2.0, this->font_h / 2.0);
	}

	// Separate render buffer for rendering the console before scaling it.
	this->render_buffer = GPU_CreateImage(this->console_resolution_x, this->console_resolution_y, GPU_FORMAT_RGBA);
	GPU_LoadTarget(this->render_buffer);

	// Load shaders
	this->crt_shader_block = loadShaderProgram(&this->crt_shader_id, cfg->shader_path_crt);
	this->text_shader_block = loadShaderProgram(&this->text_shader_id, cfg->shader_path_text);

	// Cursor clock for blinking
	this->blink_interval = cfg->blink_interval;
	this->cursor_clock = GetTickCount64();
	this->show_cursor = true;

	// Load the color scheme
	for (int i = 0; i < 16; i++)
	{
		this->color_scheme[i] = ConsoleColor(cfg->color_scheme[i].r, cfg->color_scheme[i].g, cfg->color_scheme[i].b);
	}
	// Clear the console
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
	if (fromx < 0)
		fromx = 0;
	if (fromx > this->console_w)
		fromx = this->console_w - 1;
	if (fromy < 0)
		fromy = 0;
	if (fromy > this->console_h)
		fromy = this->console_h - 1;

	for (int y = fromy; y <= toy; y++)
	{
		for (int x = fromx; x <= tox; x++)
		{
			this->PlaceChar(x, y, ' ', this->default_back_color, this->default_back_color);
		}
	}
}

void Console::LimitCursor()
{
	// Limit checks for cursor_x and cursor_y, don't go beyond the screen!
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
	// Scroll up by 1 line
	memcpy(this->buffer, (void*)((uintptr_t)this->buffer + sizeof(char) * 1 * this->console_w), this->console_w * (this->console_h - 1));
	memcpy(this->attrib_buffer, (void*)((uintptr_t)this->attrib_buffer + sizeof(char) * this->console_w), this->console_w * (this->console_h - 1));
	// Empty last line
	memset((void*)((uintptr_t)this->buffer + sizeof(char) * (this->console_h - 1) * this->console_w), ' ', this->console_w);
	memset((void*)((uintptr_t)this->attrib_buffer + sizeof(char) * (this->console_h - 1) * this->console_w), CONSTRUCT_ATTRIBUTE(this->default_fore_color, this->default_back_color), this->console_w);
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
	if (this->cursor_x >= this->console_w)
	{
		this->cursor_x = 0;
		this->cursor_y += 1;
	}
	if (c == '\n')
	{
		this->cursor_y += 1;
		this->cursor_x = 0;
	}
	else if (c == '\r')
	{
		// Don't print
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
		this->cursor_x += 4;
	}
	else
	{
		this->PlaceChar(this->cursor_x, this->cursor_y, chr, fore_color, back_color);
		this->cursor_x += 1;
		if (this->cursor_x >= this->console_w)
		{
			this->cursor_x = 0;
			this->cursor_y += 1;
		}
	}

	if (this->cursor_y >= this->console_h)
	{
		this->Scroll();
	}

	this->LimitCursor();

}

void Console::PlaceChar(int x, int y, unsigned char c, int fore_color, int back_color)
{
	if (x < 0 || y < 0 || x >= this->console_w || y >= this->console_h) return;
	this->buffer[y * this->console_w + x] = c;
	this->attrib_buffer[y * this->console_w + x] = CONSTRUCT_ATTRIBUTE(fore_color, back_color);
}

void Console::Puts(std::string s)
{
	for (int i = 0; i < s.length(); i++)
	{
		this->PutChar(s[i]);
	}
}

void Console::Render(GPU_Target* t, int xloc, int yloc, float scale)
{
	GPU_Clear(this->render_buffer->target);
	float time = GetTickCount64();
	// Enable text shader
	GPU_ActivateShaderProgram(this->text_shader_id, &this->text_shader_block);
	// Blink Cursor
	if (time - this->cursor_clock > this->blink_interval)
	{
		this->show_cursor = !this->show_cursor;
		this->cursor_clock = time;
	}
	for (int y = 0; y < this->console_h; y++)
	{
		for (int x = 0; x < this->console_w; x++)
		{
			int xcur = (x * this->font_w);
			int ycur = (y * this->font_h);
			unsigned char c = this->buffer[y * this->console_w + x];
			// Lower 4 bits = text color, higher 4 bits = background color
			int text_color = this->attrib_buffer[y * this->console_w + x] & 0xF;
			int back_color = (this->attrib_buffer[y * this->console_w + x] & 0xF0) >> 4;
			// If its backcolor, don't draw, we already pass back color to the shader
			if (back_color != this->default_back_color)
			{
				// If it is not back_color draw a block (ASCII 219) with the backcolor
				GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, this->color_scheme[back_color].returnArray());
				GPU_Blit(this->char_blocks[219], NULL, this->render_buffer->target, xcur + font_w / 2, ycur + font_h / 2);
			}
			GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, this->color_scheme[text_color].returnArray());
			GPU_Blit(this->char_blocks[c], NULL, this->render_buffer->target, xcur + font_w / 2, ycur + font_h / 2);
		}
	}
	// Draw the cursor
	if (this->show_cursor)
	{
		GPU_SetUniformfv(GPU_GetUniformLocation(this->text_shader_id, "text_color"), 3, 1, this->color_scheme[this->default_fore_color].returnArray());
		GPU_Blit(this->char_blocks[219], NULL, this->render_buffer->target, this->cursor_x * this->font_w + font_w / 2, this->cursor_y * this->font_h + font_h / 2);
	}

	// Now apply the CRT effect shader
	// The CRT effect shader applies CRT warp effect, CRT phosphor glow scanline effect, and noise.
	GPU_ActivateShaderProgram(this->crt_shader_id, &this->crt_shader_block);
	float resolution[2] = { (float)t->w, (float)t->h };
	GPU_SetUniformf(GPU_GetUniformLocation(this->crt_shader_id, "time"), time/1000.0);
	GPU_SetUniformfv(GPU_GetUniformLocation(this->crt_shader_id, "resolution"), 2, 1, (float*)&resolution);
	// Pass the back color to give the glow accent
	GPU_SetUniformfv(GPU_GetUniformLocation(this->crt_shader_id, "back_color"), 3, 1, this->color_scheme[this->default_back_color].returnArray());
	// Pass the CRT background image to blend with
	GPU_SetShaderImage(this->crt_background, GPU_GetUniformLocation(this->crt_shader_id, "crt_background"), 1);
	// Now blit to screen!
	GPU_BlitScale(this->render_buffer, NULL, t, xloc + (this->render_buffer->w / 2) * scale, yloc + (this->render_buffer->h / 2) * scale, scale, scale);
	GPU_DeactivateShaderProgram();
}