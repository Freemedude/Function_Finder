#pragma once

#include "function_finder/function_finder.hpp"

// This is used as a search term for functions
#define CONSOLE_COMMAND
#define OTHER_CONSOLE_COMMAND

CONSOLE_COMMAND
inline int print_hello_and_ret_2()
{
    printf("hello!\n");
    return 2;
}

OTHER_CONSOLE_COMMAND // This is a pretty useless function!
inline int print_hello_and_ret_3(int a, int b = 5)
{
    printf("hello! %d %d\n", a, b);
    return 3;
}