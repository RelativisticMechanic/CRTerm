#include <iostream>
#include "ArgumentParser.h"

inline bool expectArg(int current, int argc, std::string error)
{
	if (!(current < argc))
	{
		std::cerr << "[ERROR] expected argumnet for: " << error << std::endl;
		exit(-1);
	}
}

inline void unrecognisedArg(std::string arg)
{
	std::cerr << "[ERROR] unrecognised argument: " << arg << std::endl;
	exit(-1);
}

inline void invalidArg(std::string arg, std::string value)
{
	std::cerr << "[ERROR] Invalid argument to '" << arg << "': " << value << std::endl;
	exit(-1);
}

void ArgumentParser::Parse(int argc, char** argv)
{
	for (int i = 1; i < argc; i++)
	{
		std::string current_arg = std::string(argv[i]);
		if (this->parser_state == ARGUMENT_ATTR)
		{
			this->argmap[this->processing_arg] = current_arg;
			this->parser_state = ARGUMENT_NORMAL;
		}
		else 
		{
			if (current_arg[0] != '-')
			{
				/* If this is the first argument and doesnt begin with a dash, just store it */
				if (this->parser_state == ARGUMENT_LIST_BEGIN)
				{
					this->argmap["first_argument"] = current_arg;
					this->argmap[this->first_arg] = current_arg;
				}
				else
				{
					unrecognisedArg(current_arg);
				}
			}
			else
			{
				/* Remove the '-' */
				current_arg.erase(0, 1);
				if (std::find(this->expected_args.begin(), this->expected_args.end(), current_arg) != this->expected_args.end())
				{
					this->parser_state = ARGUMENT_ATTR;
					this->processing_arg = current_arg;
				}
				else
				{
					unrecognisedArg(current_arg);
				}
			}
		}
	}
}

void ArgumentParser::AddArgument(std::string arg, bool first_arg)
{
	this->expected_args.push_back(arg);
	if (first_arg)
	{
		this->first_arg = arg;
	}
}

void ArgumentParser::GetArgument(std::string arg, std::string& result)
{
	if (this->argmap.find(arg) != this->argmap.end())
	{
		result = this->argmap[arg];
	}
}

void ArgumentParser::GetArgument(std::string arg, int& result)
{
	if (this->argmap.find(arg) != this->argmap.end())
	{
		try
		{
			result = std::stoi(this->argmap[arg]);
		}
		catch (...)
		{
			invalidArg(arg, this->argmap[arg]);
		}
	}
}

void ArgumentParser::GetArgument(std::string arg, float& result)
{
	if (this->argmap.find(arg) != this->argmap.end())
	{
		try
		{
			result = std::stof(this->argmap[arg]);
		}
		catch (...)
		{
			invalidArg(arg, this->argmap[arg]);
		}
	}
}