#pragma once

#include "function_finder/function_finder.hpp"

// These is used as a search term for functions
#define CONSOLE_COMMAND
#define OTHER_CONSOLE_COMMAND

// This command WILL be included in the search, because we're asking for everything in the 
// examples/cmd_client directory,
CONSOLE_COMMAND
inline int print_hello_and_ret_2()
{
    printf("hello!\n");
    return 2;
}

// This is an example of a function that WONT be included in the search for CONSOLE_COMMAND
OTHER_CONSOLE_COMMAND 
inline int print_hello_and_ret_3(int a, int b = 5)
{
    printf("hello! %d %d\n", a, b);
    return 3;
}