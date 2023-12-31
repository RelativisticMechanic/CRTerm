#ifndef CRTERM_H
#define CRTERM_H

#include <string>

#define CRTERM_VERSION_STRING "CRTerm 0.5.5"
#define CRTERM_CREDIT_STRING "\xC2\xA9 Siddharth Gautam, 2023\nThis software comes with NO WARRANTY.\n"

inline bool endsWith(std::string const& value, std::string const& ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

#endif