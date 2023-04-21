#include "output.hpp"
#include "common.hpp"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstring>

bool string_to_arg_list(const char *args, std::vector<std::string> &out_args)
{
    const char *cur = args;
    while (*cur != 0)
    {
        cur = skip_whitespace(cur);
        std::string word;
        int count = get_string(cur, word);
        if (!count)
        {
            // No moreeeee
            break;
        }
        cur += count;
        out_args.push_back(word);
    }

    return true;
}

int main(int arg_c, const char **args)
{
    Command_Map commands = init_commands();

    std::string line;
    printf(">");
    while (std::getline(std::cin, line))
    {
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
            printf(">");
            continue;
        }

        std::vector<std::string> args;
        string_to_arg_list(line.c_str(), args);

        if (args.size() == 0)
        {
            printf("Write something, idiot\n");
            printf(">");
            continue;
        }

        std::string function_name = args[0];

        args.erase(args.begin(), args.begin() + 1);

        if (commands.contains(function_name))
        {
            Value v;
            commands[function_name](args, v);
            output_value_to_stream(std::cout, v);
            printf("\n");
        }
        else
        {
            printf("Unknown command \"%s\". Try \"help\" to get a list of commands.\n", function_name.c_str());
        }
        printf(">");
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
std::string just_return(std::string str = "hello")
{
    return str;
}