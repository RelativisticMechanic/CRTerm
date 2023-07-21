#ifndef WIN32CLIPBOARD_H
#define WIN32CLIPBOARD_H

#include <Windows.h>
#include <string>

void CopyToClipboard(std::string s);
std::string GetFromClipboard(void);

#endif