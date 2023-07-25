#ifndef DEBUG_STUFF_H
#define DEBUG_STUFF_H

#include <string>

inline std::string string_to_hex(const std::string& input)
{
    static const char hex_digits[] = "0123456789ABCDEF";

    std::string output;
    output.reserve(input.length() * 2);
    for (unsigned char c : input)
    {
        if (isalnum(c) || c == ' ')
        {
            output.push_back(c);
        }
        else {
            output.push_back(hex_digits[c >> 4]);
            output.push_back(hex_digits[c & 15]);
        }
    }
    return output;
}

#endif