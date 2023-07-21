#ifndef CONPTY_H
#define CONPTY_H

#include <Windows.h>

HRESULT SpawnProcessinConsole(wchar_t szCommand[], HPCON, PROCESS_INFORMATION*);
HRESULT CreatePseudoConsoleAndPipes(HPCON* phPC, HANDLE* phPipeIn, HANDLE* phPipeOut, SHORT consoleWidth, SHORT consoleHeight);

#endif