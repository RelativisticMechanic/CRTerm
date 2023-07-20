// CRTerm.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include "SDL_gpu.h"
#include "Shaders.h"
#include "Console.h"
#include "VT100.h"
#include "ConPTY.h"
#include "CRTermConfig.h"

#undef main

// VT100 pointer to be used by both the threads.
VT100* vt100_term = NULL;

// Functions that listens to the program output.
void __cdecl outputListener(LPVOID hPipe)
{
	const DWORD BUFF_SIZE{ 4096 };
	char szBuffer[BUFF_SIZE]{};
	DWORD dwBytesWritten{};
	DWORD dwBytesRead{};
	BOOL fRead{ FALSE };
	do
	{
		// Read from the pipe
		fRead = ReadFile(hPipe, szBuffer, BUFF_SIZE, &dwBytesRead, NULL);
		// Write received text to the Console
		for (int i = 0; i < dwBytesRead; i++)
		{
			if (szBuffer[i] == '\0')
				break;
			vt100_term->VT100Take(szBuffer[i]);
		}

	} while (fRead && dwBytesRead >= 0);
}

int main()
{
	// Read configuration
	CRTermConfiguration* cfg = new CRTermConfiguration("default.json");

	// Set the screen resolution
	int resolution_x = (cfg->font_width * cfg->font_scale * cfg->console_width);
	int resolution_y = (cfg->font_height * cfg->font_scale * cfg->console_height);

	// Create the screen
	GPU_SetDebugLevel(GPU_DEBUG_LEVEL_MAX);
	GPU_Target* screen = GPU_Init(resolution_x, resolution_y, GPU_DEFAULT_INIT_FLAGS);
	if (screen == NULL)
	{
		return 1;
	}

	/*
		Start of Win32 Code
	*/
	// Create the borderless window
	LONG lStyle = GetWindowLong(GetActiveWindow(), GWL_STYLE);
	lStyle &= ~(WS_EX_LAYERED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
	SetWindowLong(GetActiveWindow(), GWL_STYLE, lStyle);
	SetWindowLong(GetActiveWindow(), GWL_EXSTYLE, GetWindowLong(GetActiveWindow(), GWL_EXSTYLE) | WS_EX_LAYERED);
	SetWindowText(GetActiveWindow(), (LPCWSTR)L"CRTerm");

	// Make the black parts of the window transparent
	SetLayeredWindowAttributes(GetActiveWindow(), RGB(0, 255, 0), 255, LWA_COLORKEY | LWA_ALPHA);
	
	// Create the pseudo console and the pipes
	HPCON hPC{ INVALID_HANDLE_VALUE };
	HANDLE hPipeIn{ INVALID_HANDLE_VALUE };
	HANDLE hPipeOut{ INVALID_HANDLE_VALUE };
	HRESULT hr = CreatePseudoConsoleAndPipes(&hPC, &hPipeIn, &hPipeOut, cfg->console_width, cfg->console_height);
	if (hr != S_OK)
	{
		fprintf(stderr, "Unable to create Win32 Console.\n");
		return 1;
	}

	/*
		End of Win32 Code
	*/
	SDL_Event ev;
	bool done = false;
	Console* console = new Console(cfg);
	vt100_term = new VT100(console);
	console->Puts("Terminal initialized.\n");
	console->Puts("Loading shell...\n");
	// Link Pipe to Listener thread
	HANDLE hPipeListenerThread{ reinterpret_cast<HANDLE>(_beginthread(outputListener, 0, hPipeIn)) };
	// Start the process
	std::wstring process_name = std::wstring(cfg->shell_command.begin(), cfg->shell_command.end());
	SpawnProcessinConsole((wchar_t*)process_name.c_str(), hPC);
	// Wait
	Sleep(200);

	while (!done)
	{
		while (SDL_PollEvent(&ev))
		{
			if (ev.type == SDL_QUIT)
			{
				done = true;
			}
			if (ev.type == SDL_TEXTINPUT)
			{
				//c->PutChar(ev.text.text[0]);
				char c = ev.text.text[0];
				WriteFile(hPipeOut, &c, 1, NULL, NULL);
			}
			if (ev.type == SDL_KEYDOWN)
			{
				// Special key
				if (ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_RETURN2)
				{
					const char out = 13;
					WriteFile(hPipeOut, &out, 1, NULL, NULL);
				}
				if (ev.key.keysym.sym == SDLK_BACKSPACE)
				{
					const char out = '\b';
					WriteFile(hPipeOut, &out, 1, NULL, NULL);
				}

				if (ev.key.keysym.sym == SDLK_UP)
				{
					const char* out = "\x1B[A";
					WriteFile(hPipeOut, out, strlen(out), NULL, NULL);
				}

				if (ev.key.keysym.sym == SDLK_RIGHT)
				{
					const char* out = "\x1B[D";
					WriteFile(hPipeOut, out, strlen(out), NULL, NULL);
				}

				if (ev.key.keysym.sym == SDLK_DOWN)
				{
					const char* out = "\x1B[B";
					WriteFile(hPipeOut, out, strlen(out), NULL, NULL);
				}

				if (ev.key.keysym.sym == SDLK_LEFT)
				{
					const char* out = "\x1B[C";
					WriteFile(hPipeOut, out, strlen(out), NULL, NULL);
				}

				if (ev.key.keysym.sym == SDLK_ESCAPE)
				{
					const char out = 27;
					WriteFile(hPipeOut, &out, 1, NULL, NULL);
				}
			}
		}
		GPU_Clear(screen);
		console->Render(screen, 0, 0, cfg->font_scale);
		GPU_Flip(screen);
	}
	return 0;

}