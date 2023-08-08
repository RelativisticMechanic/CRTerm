#ifndef CONSOLE_FONT_H
#define CONSOLE_FONT_H

#include "SDL_gpu.h"
#include <stdint.h>

/* ASCII 219 - Block */
#define DEFAULT_INVALID_CHAR 219
/*
	ConsoleChar is UTF-32 encoded character.
	ConsoleAttrib's encoding is given in Console.
*/
typedef uint32_t ConsoleChar;
typedef uint32_t ConsoleAttrib;

class ConsoleFont
{
public:
	ConsoleFont() { };
	virtual GPU_Image* GetGlyph(ConsoleChar c) { return NULL; };
	virtual int GetXAdvance() { return 0; };
	virtual int GetYAdvance() { return 0; };
};

#endif