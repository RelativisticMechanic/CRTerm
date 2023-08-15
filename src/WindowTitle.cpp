#include "WindowTitle.h"

std::string CRTerm_title_prefix = CRTERM_DEFAULT_TITLE_PREFIX;
std::string CRTerm_title = "";

void CRTermSetWindowTitle(SDL_Window* window, std::string title)
{
	CRTerm_title = title;
	std::string window_title = CRTerm_title_prefix + " " + CRTerm_title;
	SDL_SetWindowTitle(window, window_title.c_str());
}

void CRTermSetWindowTitlePrefix(SDL_Window* window, std::string prefix)
{
	CRTerm_title_prefix = prefix;
	CRTermSetWindowTitle(window, CRTerm_title);
}
