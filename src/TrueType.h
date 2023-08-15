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
#define TRUETYPE_DEFAULT_CACHE_SIZE 256

#include "ft2build.h"
#include FT_FREETYPE_H

#include "SDL_gpu.h"
#include "ConsoleFont.h"
#include "LRUCache.h"

typedef struct 
{
	SDL_Surface* surface;
	GPU_Image* image;
	uint8_t* data;
} GlyphData;

class FreeTypeFont : public ConsoleFont
{
public:

	FreeTypeFont(std::string filename, int size);
	int GetXAdvance() override;
	int GetYAdvance() override;
	GPU_Image* GetGlyph(ConsoleChar c) override;
	~FreeTypeFont();

private:
	LRUCache<ConsoleChar, GlyphData*>* unicode_cache;
	FT_Library ft_lib;
	FT_Face ft_face;
	FT_Face ft_fallback;
	GlyphData glyphs[256];
	int mono_width;
	int mono_height;
	void GenerateGlyph(GlyphData* glyph, uint32_t codepoint);
};
#endif
