#include "PNGFont.h"
#include <iostream>

PNGBitmapFont::PNGBitmapFont(std::string filename, int char_width, int char_height)
{
	this->char_width = char_width;
	this->char_height = char_height;

	this->font_bitmap = GPU_LoadImage(filename.c_str());

	if (!this->font_bitmap)
	{
		std::cout << "Error failed to load font bitmap: " << filename << std::endl;
		exit(-1);
	}

	int chars_per_row = font_bitmap->w / this->char_width;
	for (int i = 0; i < 256; i++)
	{
		GPU_Rect r;
		r.x = (i % chars_per_row) * this->char_width;
		r.y = (i / chars_per_row) * this->char_height;
		r.w = this->char_width;
		r.h = this->char_height;
		this->characters[i] = GPU_CreateImage(this->char_width, this->char_height, GPU_FORMAT_RGBA);
		GPU_Target* tg = GPU_LoadTarget(this->characters[i]);
		if (tg != this->characters[i]->target || tg == NULL)
		{
			std::cerr << "Warning: GPU_LoadTarget Failed!" << std::endl;
			exit(-1);
		}
		GPU_Blit(font_bitmap, &r, tg, this->char_width / 2.0, this->char_height / 2.0);
	}
}

int PNGBitmapFont::GetXAdvance()
{
	return this->char_width;
}

int PNGBitmapFont::GetYAdvance()
{
	return this->char_height;
}

GPU_Image* PNGBitmapFont::GetGlyph(ConsoleChar c)
{
	return this->characters[c];
}

PNGBitmapFont::~PNGBitmapFont()
{
	for (int i = 0; i < 256; i++)
	{
		GPU_FreeImage(this->characters[i]);
	}
	GPU_FreeImage(this->font_bitmap);
}