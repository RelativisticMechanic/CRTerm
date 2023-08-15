#pragma once

#ifndef WINDOW_TITLE_H
#define WINDOW_TITLE_H

#include "SDL.h"
#include "CRTerm.h"
#include <string>

#define CRTERM_DEFAULT_TITLE_PREFIX "Gautam's CRTerm -"
#define CRTERM_DEFAULT_MUSIC_TITLE_PREFIX "Gautam's CRTerm (Press END to stop) -"

void CRTermSetWindowTitle(SDL_Window* window, std::string title);
void CRTermSetWindowTitlePrefix(SDL_Window* window, std::string prefix);

#endif