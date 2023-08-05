#ifndef CRTERM_TRUETYPE_IMPL
#define CRTERM_TRUETYPE_IMPL

#include <cstdint>
#include <string>

#include "ft2build.h"
#include FT_FREETYPE_H

#include "SDL_gpu.h"

class FreeTypeFont
{
public:
	GPU_Image* glyph_images[256];

	FreeTypeFont(std::string filename, int size);
	~FreeTypeFont();

private:
	FT_Library ft_lib;
	FT_Face ft_face;
	SDL_Surface* glyph_surfaces[256];
	uint8_t* glyph_data[256];
	int mono_width;
	int mono_height;
	int advance_width;
};
#endif
