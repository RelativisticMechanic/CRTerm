#include "Win32ClipBoard.h"

void CopyToClipboard(std::string s)
{
	// Copy to clipboard
	const char* result_cstr = s.c_str();
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, s.length());
	memcpy(GlobalLock(hMem), result_cstr, s.length());
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

/* Thanks to https://stackoverflow.com/questions/14762456/getclipboarddatacf-text */
std::string GetFromClipboard(void)
{
	std::string result = "";
	// Try opening the clipboard
	if (!OpenClipboard(nullptr))
		return result;

	// Get handle of clipboard object for ANSI text
	HANDLE hData = GetClipboardData(CF_TEXT);
	if (hData == nullptr)
		return result;

	// Lock the handle to get the actual text pointer
	char* pszText = static_cast<char*>(GlobalLock(hData));
	if (pszText == nullptr)
		return result;

	// Save text in a string class instance
	std::string text(pszText);

	// Release the lock
	GlobalUnlock(hData);

	// Release the clipboard
	CloseClipboard();

	return text;
}