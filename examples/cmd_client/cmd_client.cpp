#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

#include "function_finder/function_finder.hpp"
#include "console_commands_out.hpp"
#include "cmd_client.hpp"

bool convert_string_to_arg_list(std::string_view source, std::vector<std::string> &out_args);
void run_help_command(std::string_view line, Function_Map &commands);
void print_unknown_command(std::string_view command_name);

const std::string WHERE_COMMAND = "where";
const std::string HELP_COMMAND = "help";
const std::string EXIT_COMMAND = "exit";

int main(int arg_c, const char **args)
{
	Function_Map commands;
	init_console_commands(commands);

	std::string line;
	while (true)
	{
		std::cout << ">";
		auto more = (bool)std::getline(std::cin, line);
		if (!more)
		{
			break;
		}

		if (line == EXIT_COMMAND)
		{
			break;
		}

		if (line.starts_with(WHERE_COMMAND))
		{
			if (line.size() + 1 == WHERE_COMMAND.size())
			{
				std::cout << "Type 'where <command>' to see where a command is implemented\n";
				continue;
			}
			
			std::string view(line.begin() + WHERE_COMMAND.size() + 1, line.end());

			if (commands.contains(view))
			{
				auto cmd = commands[view];
				std::cout << std::format("You can find command \"{}\" at {} : L{}\n", view, cmd.file, cmd.line);
			}
			else
			{
				std::cout << std::format("Unknown command \"{}\". Try \"help\" to get a list of commands.\n", view);
			}
			continue;
		}

		if (line.starts_with(HELP_COMMAND))
		{
			run_help_command(line, commands);
			continue;
		}

		std::vector<std::string> args;
		bool success = convert_string_to_arg_list(line.c_str(), args);
		if (!success)
		{
			std::cout << "Failed to parse inputs, try again\n";
			continue;
		}

		if (args.size() == 0)
		{
			std::cout << "Write the name of a command followed by the arguments to run the command!\n";
			std::cout << "Try writing 'help' to get a list of commands!";
			continue;
		}

		std::string function_name = args[0];
		args.erase(args.begin(), args.begin() + 1);

		if (commands.contains(function_name))
		{
			Value v;
			bool success = commands[function_name].function(args, v);
			if (!success)
			{
				continue;
			}

			if (v.type != Value_Type::VOID)
			{
				std::cout << value_to_string(v);
			    std::cout << "\n";
			}
		}
		else
		{
			print_unknown_command(function_name);
		}
	}

	return 0;
}


void print_unknown_command(std::string_view command_name)
{
	std::cout << std::format("Unknown command \"{}\". Try \"help\" to get a list of commands.\n", command_name);
}

void run_help_command(std::string_view line, Function_Map &commands)
{
	if (line == HELP_COMMAND)
	{
		std::cout << "These are the commands:\n";
		for (const auto &cmd : commands)
		{
			std::cout << " - " << cmd.first << "\n";
		}

		std::cout << "You can get more details about a command with 'help <command>' or 'where <command>'\n";
		return;
	}

	// Getting here indicates there is more
	std::string command_name(line.begin() + HELP_COMMAND.size() + 1, line.end());

	// For some reason .contains doesn't support string_view...
	if (commands.contains(command_name))
	{
		auto cmd = commands[command_name];
		std::cout << std::format("Command \"{}\" \n\tNote: {}\n\tUsage: '{} ", cmd.name, cmd.note, cmd.name);

		// These are the required arguments:
		for (int i = 0; i < cmd.num_required_args; i++)
		{
			std::cout << std::format("<{} : {}>", cmd.arguments[i].name, type_to_readable_string(cmd.arguments[i].type));
			if (i < cmd.num_required_args - 1)
			{
				std::cout << " ";
			}
		}

		// If there are optional arguments, give it a space in between.
		if (cmd.num_optional_args > 0)
		{
			std::cout << " ";
		}

		// Thesee are the optional arguments. They therefore include the default value.
		for (int i = cmd.num_required_args; i < cmd.arguments.size(); i++)
		{
			std::cout << std::format("[{} : {} = {}]",
				cmd.arguments[i].name, type_to_readable_string(cmd.arguments[i].type), value_to_string(cmd.arguments[i].default_value));

			// Check if there are more arguments.
			if (i < cmd.arguments.size() - 1)
			{
				std::cout << " ";
			}
		}

		// Return type
		std::cout << std::format(" -> {}'\n", type_to_readable_string(cmd.return_type));
	}
	else
	{
		print_unknown_command(command_name);
	}
}


bool convert_string_to_arg_list(std::string_view source, std::vector<std::string> &out_args)
{
	size_t max_length = source.size();
	size_t length = 0;

	while (length < max_length && source[length] != 0)
	{
		auto current = advance(source, length);
		auto after_whitespace = skip_whitespace(current);
		length += current.size() - after_whitespace.size();
		current = after_whitespace;
		std::string word;
		size_t count = get_string(current, word);
		if (!count)
		{
			// The string ended in whitespace.
			return false;
		}
		auto after_word = advance(current, count);
		length += count;

		out_args.push_back(word);
	}

	return true;
}


CONSOLE_COMMAND
/* Hello there. This function adds two numbers. a, and b.
DEPRECATED
*/
int add(int a, int b)
{
	return a + b;
}

CONSOLE_COMMAND
// A float version of the normnhnhal add function. Probably the only one you need.
float add_f(float a, float b)
{
	return a + b;
}

CONSOLE_COMMAND // String-appends a string 'a' with the float 'b'. Just a tester lmao
std::string append(std::string a, float b)
{
	printf("%s\n", a.c_str());
	std::string result = a + std::to_string(b);
	return result;
}

CONSOLE_COMMAND // Does some fkin nonsense
void complex(std::string base, int num_prints, bool capitalize = false, std::string to_print = "cringe", int indents = 4)
{
	if (capitalize)
	{
		std::string upper;
		upper.resize(to_print.size());
		std::transform(to_print.begin(), to_print.end(), upper.begin(), [](char c)
			{ return std::toupper(c); });
		to_print = upper;
	}

	for (int i = 0; i < num_prints; i++)
	{
		for (int j = 0; j < indents; j++)
		{
			printf(" ");
		}
		printf("%s%s\n", base.c_str(), to_print.c_str());
	}
}

CONSOLE_COMMAND // Just returns the input value
std::string just_return(std::string str = "hello")
{
	return str;
}

CONSOLE_COMMAND // Squares the second parameter. Ignores the first. Very useful for squaring needs!
int multiply(int a, double b = 2.1)
{
	return b * b;
}