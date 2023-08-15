#include "TrueType.h"
#include <iostream>
#include <cassert>

void DestroyGlyph(GlyphData* glyph)
{
    /* First destroy the surface */
    SDL_FreeSurface(glyph->surface);
    /* Then the GPU_IMage */
    GPU_FreeImage(glyph->image);
    /* Then the data */
    free(glyph->data);
}

/* For freeing Glyphs dynamically created */
void DestroyDynamicGlyph(GlyphData* glyph)
{
    DestroyGlyph(glyph);
    free(glyph);
}

FreeTypeFont::FreeTypeFont(std::string filename, int size)
{
    FT_Error err = FT_Init_FreeType(&ft_lib);

    err = FT_New_Face(ft_lib, filename.c_str(), 0, &ft_face);
    if (err)
    {
        std::cerr << "Unable to load TrueType font: " << filename << std::endl;
        exit(-1);
    }
    /* Load up the unifont */
    err = FT_New_Face(ft_lib, "unifont.ttf", 0, &ft_fallback);

    if (err)
    {
        std::cerr << "Unable to load fallback font! (unifont.ttf)" << std::endl;
        exit(-1);
    }

    this->mono_height = size;
    this->mono_width = size;

    /* Set pixel size */
    err = FT_Set_Pixel_Sizes(ft_face, mono_width, mono_height);
    err = FT_Set_Pixel_Sizes(ft_fallback, mono_width, mono_height);

    /* Set unicode encoding */
    FT_Select_Charmap(ft_face, ft_encoding_unicode);
    FT_Select_Charmap(ft_fallback, ft_encoding_unicode);

    /* Load up the first 256 glyphs */
    for (int i = 0; i < 256; i++)
    {
        this->GenerateGlyph(&(this->glyphs[i]), i);
    }

    this->unicode_cache = new LRUCache<ConsoleChar, GlyphData*>(TRUETYPE_DEFAULT_CACHE_SIZE, DestroyDynamicGlyph);
}

int FreeTypeFont::GetXAdvance()
{
    return this->mono_width / 2;
}

int FreeTypeFont::GetYAdvance()
{
    return this->mono_height;
}

void FreeTypeFont::GenerateGlyph(GlyphData* glyph, uint32_t codepoint)
{
    /* Load up the glyph */
    FT_UInt glyph_index = FT_Get_Char_Index(ft_face, codepoint);
    FT_Face face_to_use = ft_face;
    if (!glyph_index)
    {
        /* Failed, resort to fallback font (GNU Unifont) */
        glyph_index = FT_Get_Char_Index(ft_fallback, codepoint);
        face_to_use = ft_fallback;
    }

    FT_Load_Glyph(face_to_use, glyph_index, FT_LOAD_DEFAULT);
    FT_Render_Glyph(face_to_use->glyph, FT_RENDER_MODE_NORMAL);
    uint8_t* src = face_to_use->glyph->bitmap.buffer;

    /* Allocate a fixed size buffer, as TrueType fonts are not fixed size but we are. */
    uint8_t* letter_buf = (uint8_t*)calloc(mono_height * mono_width, sizeof(uint8_t));
    glyph->data = letter_buf;

    /* Place the newly rendered glyph into our fixed size bitmap buffer */

    int height = face_to_use->glyph->bitmap.rows;
    int width = face_to_use->glyph->bitmap.width;
    /* Center the bitmap horizontally into our fixed size buffer */
    int offsetx = (mono_width - width) / 2;
    /* TODO: Why does this formula work? */
    int offsety = (mono_height - face_to_use->glyph->bitmap_top - (mono_height / 4));

    /* Copy data from source buffer into our fixed size buffer */
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            uint8_t val = src[i * width + j];

            int idx = (offsety + i) * mono_width + (offsetx + j);
            if (idx < mono_width * mono_height && idx > 0)
            {
                letter_buf[idx] = val;
            }
        }
    }
    /* Create SDL_Surface from bitmap */
    glyph->surface = SDL_CreateRGBSurfaceFrom(letter_buf, mono_width, mono_height, 8, mono_width, 0, 0, 0, 0xff);

    /* Set the palette colors accordingly 0 = black, 255 = white */
    SDL_Color colors[256];
    for (int j = 0; j < 256; j++)
    {
        colors[j].r = colors[j].g = colors[j].b = j;
        colors[j].a = j;
    }
    SDL_SetPaletteColors(glyph->surface->format->palette, colors, 0, 256);
    /* Make black transparent */
    SDL_SetColorKey(glyph->surface, SDL_TRUE, 0);

    /* Convert to GPU_Image */
    glyph->image = GPU_CopyImageFromSurface(glyph->surface);
}

/* This function is executed every redraw, it is important. */
GPU_Image* FreeTypeFont::GetGlyph(ConsoleChar c)
{
    if (c < 256)
    {
        return glyphs[c].image;
    }
    else
    {
        GlyphData* glyph_data;
        if (this->unicode_cache->Get(c, glyph_data))
        {
            return glyph_data->image;
        }
        else
        {
            /* Doesn't exist, generate one. */
            glyph_data = (GlyphData*)calloc(1, sizeof(GlyphData));
            GenerateGlyph(glyph_data, c);
            this->unicode_cache->Add(c, glyph_data);
            return glyph_data->image;
        }
    }
}



FreeTypeFont::~FreeTypeFont()
{
    for (int i = 0; i < 256; i++)
    {
        DestroyGlyph(&(this->glyphs[i]));
    }
}