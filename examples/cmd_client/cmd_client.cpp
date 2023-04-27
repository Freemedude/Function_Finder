#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

#include "function_finder/function_finder.hpp"
#include "console_commands_out.hpp"
#include "other_console_commands_out.hpp"
#include "cmd_client.hpp"

bool string_to_arg_list(std::string_view source, std::vector<std::string> &out_args)
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
		length += current.size() - after_word.size();

		out_args.push_back(word);
	}

	return true;
}

int main(int arg_c, const char **args)
{
	Command_Map trash;
	Command_Map commands;
	init_console_commands(commands);
	init_other_console_commands(trash);

	std::string line;
	while (true)
	{
		printf(">");
		auto more = (bool)std::getline(std::cin, line);
		if (!more)
		{
			break;
		}

		if (line == "exit")
		{
			break;
		}

		if (line.starts_with("where"))
		{
			if (line.size() + 1 == (sizeof("where")))
			{
				printf("Type 'where <command>' to see where a command is implemented\n");
				continue;
			}
			std::string view(line.begin() + sizeof("where"), line.end());

			if (commands.contains(view))
			{
				auto cmd = commands[view];
				printf("You can find command \"%s\" at %s : L%zu\n", view.data(), cmd.file.c_str(), cmd.line);
			}
			else
			{
				printf("Unknown command \"%s\". Try \"help\" to get a list of commands.\n", view.data());
			}
			continue;
		}

		if (line.starts_with("help"))
		{
			if (line == "help")
			{
				printf("These are the commands:\n");
				for (const auto &cmd : commands)
				{
					printf(" - %s\n", cmd.first.c_str());
				}

				printf("You can get more details about a command with 'help <command>' or 'where <command>'\n");
				continue;
			}

			// Getting here indicates there more
			std::string view(line.begin() + sizeof("help"), line.end());

			if (commands.contains(view))
			{
				auto cmd = commands[view];
				printf("Command \"%s\" \n\tNote: %s\n\tUsage: '%s ", cmd.name.c_str(), cmd.note.c_str(), cmd.name.c_str());
				for (int i = 0; i < cmd.num_required_args; i++)
				{
					printf("<%s : %s>", cmd.arguments[i].name.c_str(), type_to_readable_string(cmd.arguments[i].type).c_str());
					if (i < cmd.num_required_args - 1)
					{
						printf(" ");
					}
				}

				if (cmd.arguments.size() > cmd.num_required_args)
				{
					printf(" ");
				}

				for (int i = cmd.num_required_args; i < cmd.arguments.size(); i++)
				{
					printf("[%s : %s = ", cmd.arguments[i].name.c_str(), type_to_readable_string(cmd.arguments[i].type).c_str());

					output_value_to_stream(std::cout, cmd.arguments[i].default_value);

					printf("]");

					if (i < cmd.arguments.size() - 1)
					{
						printf(" ");
					}
				}

				printf(" -> %s'\n", type_to_readable_string(cmd.return_type).c_str());
			}
			else
			{
				printf("Unknown command \"%s\". Try \"help\" to get a list of commands.\n", view.data());
			}
			continue;
		}

		std::vector<std::string> args;
		bool success = string_to_arg_list(line.c_str(), args);
		if (!success)
		{
			printf("Failed to parse inputs, try again\n");
			continue;
		}

		if (args.size() == 0)
		{
			printf("Write something, idiot\n");
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
				output_value_to_stream(std::cout, v);
			}
			printf("\n");
		}
		else
		{
			printf("Unknown command \"%s\". Try \"help\" to get a list of commands.\n", function_name.c_str());
		}
	}

	return 0;
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