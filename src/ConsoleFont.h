#ifndef CONSOLE_FONT_H
#define CONSOLE_FONT_H

#include "SDL_gpu.h"

/*
	In future when we implement UTF-8
*/

typedef unsigned char ConsoleChar;
typedef unsigned char ConsoleAttrib;

class ConsoleFont
{
public:
	ConsoleFont() { };
	virtual GPU_Image* GetGlyph(ConsoleChar c) { return NULL; };
	virtual int GetXAdvance() { return 0; };
	virtual int GetYAdvance() { return 0; };
};

#endif