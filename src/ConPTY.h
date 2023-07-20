#ifndef CONPTY_H
#define CONPTY_H

#include <Windows.h>

HRESULT SpawnProcessinConsole(wchar_t szCommand[], HPCON hPC);
HRESULT CreatePseudoConsoleAndPipes(HPCON* phPC, HANDLE* phPipeIn, HANDLE* phPipeOut, SHORT consoleWidth, SHORT consoleHeight);

#endif