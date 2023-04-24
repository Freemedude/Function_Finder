#include "common.hpp"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

#include "output.hpp"

bool string_to_arg_list(std::string_view source, std::vector<std::string> &out_args)
{
    std::string_view current = source;
    while (current[0] != 0)
    {
        current = skip_whitespace(current);
        std::string word;
        size_t count = get_string(current, word);
        if (!count)
        {
            // The string ended in whitespace.
            break;
        }
        current = advance(current, count);
        out_args.push_back(word);
    }

    return true;
}

int main(int arg_c, const char **args)
{
    Command_Map commands;
    init_commands(commands);

    std::string line;
    while (true)
    {
        printf(">");
        auto more = (bool)std::getline(std::cin, line);
        if(!more)
        {
            break;
        }

        if (line == "exit")
        {
            break;
        }

        if (line == "help")
        {
            printf("These are the commands:\n");
            for (const auto &cmd : commands)
            {
                printf(" - %s\n", cmd.first.c_str());
            }
            printf("\n");
            continue;
        }

        if (line.starts_with("where"))
        {
            std::string view(line.begin() + sizeof("where"), line.end());

            if (commands.contains(view))
            {
                auto cmd = commands[view];
                printf("You can find command \"%s\" at %s:L%zu\n", view.data(), cmd.file.c_str(), cmd.line);
            }
            else
            {
                printf("Unknown command \"%s\". Try \"help\" to get a list of commands.\n", view.data());
            }
            continue;
        }

        if (line.starts_with("usage"))
        {
            std::string view(line.begin() + sizeof("usage"), line.end());

            if (commands.contains(view))
            {
                auto cmd = commands[view];
                printf("Command \"%s\" usage: '%s ", cmd.name.c_str(), cmd.name.c_str());
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
        string_to_arg_list(line.c_str(), args);

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
            if(!success)
            {
                continue;
            }

            if(v.type != Value_Type::VOID)
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
int add(int a, int b)
{
    return a + b;
}

CONSOLE_COMMAND
float add_f(float a, float b)
{
    return a + b;
}

CONSOLE_COMMAND
std::string append(std::string a, float b)
{
    printf("%s\n", a.c_str());
    std::string result = a + std::to_string(b);
    return result;
}

CONSOLE_COMMAND
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

CONSOLE_COMMAND
std::string just_return(std::string str = "hello")
{
    return str;
}


CONSOLE_COMMAND
int multiply(int a, double b = 2)
{
    return b * b;
}