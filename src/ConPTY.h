/*
	These are helper functions for the Windows ConPTY API, that actually make piping 
	console apps to our terminal easy.

	The code is shamelessly copied from Microsoft's official blog. Thanks Rich Turner.
*/

#ifndef CONPTY_H
#define CONPTY_H

#include <Windows.h>

HRESULT SpawnProcessinConsole(wchar_t*, HPCON, PROCESS_INFORMATION*);
HRESULT CreatePseudoConsoleAndPipes(HPCON* phPC, HANDLE* phPipeIn, HANDLE* phPipeOut, SHORT consoleWidth, SHORT consoleHeight);

#endif