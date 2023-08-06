#ifndef PNGFONT_H
#define PNGFONT_H

#include "ConsoleFont.h"
#include <string>

class PNGBitmapFont : public ConsoleFont
{
public:
	PNGBitmapFont(std::string filename, int char_width, int char_height);
	~PNGBitmapFont();

	GPU_Image* GetGlyph(ConsoleChar) override;
	int GetXAdvance() override;
	int GetYAdvance() override;

private:
	GPU_Image* characters[256];
	GPU_Image* font_bitmap;
	int char_width, char_height;
};

#endif