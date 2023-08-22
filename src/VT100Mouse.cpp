#include "VT100Mouse.h"

std::string VT100_MouseReportDefault(int x, int y, VT100_MOUSEBTN button)
{
	return "\x1B[M" + (char)((uint8_t)button) + (char)((uint8_t)x + 32) + (char)((uint8_t)y + 32);
}

std::string VT100_MouseReportURXVT(int x, int y, VT100_MOUSEBTN button)
{
	return "\x1B[" + std::to_string((uint8_t)button) + ";" + std::to_string((uint8_t)x) + ";" + std::to_string((uint8_t)y) + "M";
}

std::string VT100_ReportMouseCoordinates(int x, int y, VT100_MOUSEBTN button, VT100_MOUSEBTN_REPORTING_STYLE style)
{
	switch (style)
	{
	case VT100_MOUSEBTN_REPORTING_STYLE_DEFAULT:
		return VT100_MouseReportDefault(x, y, button);
	case VT100_MOUSEBTN_REPORTING_STYLE_URXVT:
	default:
		return VT100_MouseReportURXVT(x, y, button);
	}
}