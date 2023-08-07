/*
	This is the FreeTypeFont class. It implements the Font class using libfreetype as the backend.
	Extracts glyphs from the TTF file and stores them in a list of 256 GPU_Images.

	GPU_Images can be drawn as required.

	Note that by default it loads a 16x16 font which can be then scaled as per the user "font_scale".
	It ignores font_height and font_width attributes in the configuration JSON.
*/

#ifndef CRTERM_TRUETYPE_IMPL
#define CRTERM_TRUETYPE_IMPL

#include <cstdint>
#include <string>

#define TRUETYPE_DEFAULT_FONT_SIZE 16

#include "ft2build.h"
#include FT_FREETYPE_H

#include "SDL_gpu.h"
#include "ConsoleFont.h"

class FreeTypeFont : public ConsoleFont
{
public:

	FreeTypeFont(std::string filename, int size);
	int GetXAdvance() override;
	int GetYAdvance() override;
	GPU_Image* GetGlyph(ConsoleChar c) override;
	~FreeTypeFont();

private:
	FT_Library ft_lib;
	FT_Face ft_face;
	SDL_Surface* glyph_surfaces[256];
	GPU_Image* glyph_images[256];
	uint8_t* glyph_data[256];
	int mono_width;
	int mono_height;
};
#endif
