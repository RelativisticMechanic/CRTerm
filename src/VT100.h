#ifndef VT100_H
#define VT100_H

#include "Console.h"
#include <string>

#define VT100_ARG_STACK_SIZE 8
#define VT100_STRING_SIZE 8

typedef enum
{
	VTSTATE_NORMAL = 0, // 0
	VTSTATE_ESC, // 1
	VTSTATE_CONTROL_STRING, // 2
	VTSTATE_TAKE_STRING, // 3
	VTSTATE_PRIVATE, // 4
	VTSTATE_ATTR, // 5
	VTSTATE_ENDVAL,
	VTSTATE_PRIVATE_ENDVAL
} VT100ParseState;

typedef struct
{
	int value;
	bool empty;
} VT100Argument;

class VT100
{
private:
	Console* con;
	int fg;
	int bg;

	VT100ParseState parser_state;
	VT100Argument argument_stack[VT100_ARG_STACK_SIZE];
	std::string control_strings[VT100_STRING_SIZE];
	int control_string_idx;
	int stack_ptr;

public:
	VT100(Console*);
	void VT100Take(unsigned char);
	void VT100Putc(unsigned char);
};

#endif