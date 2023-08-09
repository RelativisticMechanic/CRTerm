#include "Win32ClipBoard.h"
#include <iostream>
#include <wchar.h>
#include <locale.h>
/* 
	codecvt is deprecated in C++17 we must implement our own. 
*/
std::string WCharStringToUTF8(const std::wstring& wide_string)
{
	if (wide_string.empty())
	{
		return "";
	}

	const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), nullptr, 0, nullptr, nullptr);
	if (size_needed <= 0)
	{
		throw std::runtime_error("WideCharToMultiByte() failed: " + std::to_string(size_needed));
	}

	std::string result(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), &result.at(0), size_needed, nullptr, nullptr);
	return result;
}

std::wstring UTF8ToWCharString(const std::string& string)
{
	if (string.empty())
	{
		return L"";
	}

	const auto size_needed = MultiByteToWideChar(CP_UTF8, 0, &string.at(0), (int)string.size(), nullptr, 0);
	if (size_needed <= 0)
	{
		throw std::runtime_error("MultiByteToWideChar() failed: " + std::to_string(size_needed));
	}

	std::wstring result(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &string.at(0), (int)string.size(), &result.at(0), size_needed);
	return result;
}

void CopyToClipboard(std::string s)
{
	/* Convert UTF-8 to WCHAR_T */
	std::wstring converted_str = UTF8ToWCharString(s);
	const wchar_t* result_cstr = converted_str.c_str();
	
	/* WCHAR_T stuff */
	int len = wcslen(result_cstr);
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
	wchar_t* buffer = (wchar_t*)GlobalLock(hMem);
	wcscpy_s(buffer, len + 1, result_cstr);

	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_UNICODETEXT, hMem);
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
	HANDLE hData = GetClipboardData(CF_UNICODETEXT);
	if (hData == nullptr)
		return result;

	// Lock the handle to get the actual text pointer
	wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
	if (pszText == nullptr)
		return result;

	// Save text in a string class instance
	std::wstring text_wchar(pszText);

	// Release the lock
	GlobalUnlock(hData);

	// Release the clipboard
	CloseClipboard();

	/* Convert from WCHAR_T to UTF-8 */
	std::string converted_text = WCharStringToUTF8(text_wchar);
	return converted_text;
}