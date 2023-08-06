#ifndef CRTERM_H
#define CRTERM_H

#define CRTERM_VERSION_STRING "CRTerm 0.3.5"
#define CRTERM_CREDIT_STRING "(C) Siddharth Gautam, 2023\nThis software comes with NO WARRANTY.\n"
#define FRAMES_PER_SEC 60

inline bool endsWith(std::string const& value, std::string const& ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

#endif