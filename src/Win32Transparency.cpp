#include "Win32Transparency.h"
#include <iostream>

void Win32SetWindowTransparency(float transparency)
{	
    HWND hwnd = GetActiveWindow();
    LONG Style = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, Style | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, (int)(transparency * 255.0), LWA_ALPHA);
}