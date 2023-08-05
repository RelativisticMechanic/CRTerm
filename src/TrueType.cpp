#include "TrueType.h"

FreeTypeFont::FreeTypeFont(std::string filename, int size)
{
    FT_Error err = FT_Init_FreeType(&ft_lib);

    err = FT_New_Face(ft_lib, filename.c_str(), 0, &ft_face);
    this->mono_height = size;
    this->mono_width = size;

    /* Create 256 SDL_Surfaces that can be blitted at request */
    err = FT_Set_Pixel_Sizes(ft_face, mono_width, mono_height);

    for (int i = 0; i < 256; i++)
    {
        FT_UInt glyph_index = FT_Get_Char_Index(ft_face, i);
        err = FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT);
        FT_Render_Glyph(ft_face->glyph, FT_RENDER_MODE_NORMAL);

        uint8_t* src = ft_face->glyph->bitmap.buffer;

        /* Allocate a fixed size buffer */
        uint8_t* letter_buf = (uint8_t*)calloc(mono_height * mono_width, sizeof(uint8_t));
        glyph_data[i] = letter_buf;

        int height = ft_face->glyph->bitmap.rows;
        int width = ft_face->glyph->bitmap.width;
        /* Center the bitmap horizontally into our fixed size buffer */
        int offsetx = (mono_width - width) / 2;
        /* Match the botom with the bottom of the fixed size buffer */
        int offsety = (mono_height - ft_face->glyph->bitmap_top - (mono_height / 4));

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
        glyph_surfaces[i] = SDL_CreateRGBSurfaceFrom(letter_buf, mono_width, mono_height, 8, mono_width, 0, 0, 0, 0xff);

        /* Set the palette colors accordingly 0 = black, 255 = white */
        SDL_Color colors[256];
        for (int j = 0; j < 256; j++)
        {
            colors[j].r = colors[j].g = colors[j].b = j;
            colors[j].a = j;
        }
        SDL_SetPaletteColors(glyph_surfaces[i]->format->palette, colors, 0, 256);
        /* Make black transparent */
        SDL_SetColorKey(glyph_surfaces[i], SDL_TRUE, 0);

        /* Convert to GPU_Image */
        glyph_images[i] = GPU_CopyImageFromSurface(glyph_surfaces[i]);
    }
}

int FreeTypeFont::GetXAdvance()
{
    return this->mono_width / 2;
}

int FreeTypeFont::GetYAdvance()
{
    return this->mono_height;
}

GPU_Image* FreeTypeFont::GetGlyph(ConsoleChar c)
{
    return this->glyph_images[c];
}

FreeTypeFont::~FreeTypeFont()
{
    for (int i = 0; i < 256; i++)
    {
        /* First destroy the surface */
        SDL_FreeSurface(this->glyph_surfaces[i]);
        /* Then the GPU_IMage */
        GPU_FreeImage(this->glyph_images[i]);
        /* Then the data */
        free(this->glyph_data[i]);
    }
}