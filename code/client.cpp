#include "output.hpp"
#include "common.hpp"
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cstring>

const char *skip_whitespace(const char *str)
{
    const char *cur = str;
    while (*cur == ' ' || *cur == '\t')
    {
        cur++;
    }
    return cur;
}

bool string_to_arg_list(const char *args, std::vector<std::string> &out_args)
{
    const char *cur = args;
    while(true)
    {
        cur = skip_whitespace(cur);
        std::string word;
        int count = get_string(args, res);
        if(!count)
        {
            // No moreeeee
            break;
        }

        
        out_args.push_back(word);
         
    }
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

        int count;
        char **args;
        string_to_arg_list(line.c_str(), count, &args);

        if (count == 0)
        {
            printf("Write something, idiot\n");
            printf(">");
            continue;
        }

        const char *func = args[0];
        if (commands.contains(func))
        {
            Value v;
            commands[func](count - 1, (const char **)args + 1, v);
            output_value_to_stream(std::cout, v);
            printf("\n");
        }
        else
        {
            printf("Dunno wtf \"\"\"%s\"\"\" is, idiot\n", func);
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
    std::string result = a + std::to_string(b);
    return result;
}

CONSOLE_COMMAND
std::string just_return(std::string str = "hello")
{
    return str;
}