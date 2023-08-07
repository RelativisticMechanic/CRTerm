/* 
	Nicely written class for parsing arguments. Heh, just kidding, I wrote this for some C++ practice.
*/

#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <string>

#include "CRTermConfig.h"

typedef enum
{
	ARGUMENT_LIST_BEGIN,
	ARGUMENT_NORMAL,
	ARGUMENT_ATTR
} ARGUMENT_PARSER_STATE;

class ArgumentParser
{
public:
	ARGUMENT_PARSER_STATE parser_state = ARGUMENT_LIST_BEGIN;
	std::unordered_map<std::string, std::string> argmap;
	std::vector<std::string> expected_args;
	std::string first_arg;
	std::string processing_arg;
	ArgumentParser(void) { };
	void Parse(int argc, char** argv);
	void AddArgument(std::string argument, bool first_arg = false);
	void GetArgument(std::string, std::string&);
	void GetArgument(std::string, float&);
	void GetArgument(std::string, int&);
	void GetArgument(std::string arg, bool& result);
};

#endif
