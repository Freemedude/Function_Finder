#include "output.hpp"
#include "common.hpp"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstring>

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

    Function_Decl a(
        "../code/client.cpp", (size_t)122, &command_add, "add", (Value_Type)3,
        // Arguments
        {
            // Argument a
            Argument(
                "a",
                (Value_Type)3,
                "Hello"),
            // Argument b
            {
                "b",
                (Value_Type)3}},
        2,
        0);

    Command_Map commands;
    init_commands(commands);
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
            commands[function_name].function(args, v);
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