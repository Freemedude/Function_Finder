#pragma once

#include "function_finder/function_finder.hpp"

// These is used as a search term for functions
#define CONSOLE_COMMAND
#define CONSOLE_COMMAND_2

// This command WILL be included in the search, because we're asking for everything in the 
// examples/cmd_client directory,
CONSOLE_COMMAND // Hello
inline int print_hello_and_ret_2()
{
    printf("hello!\n");
    return 2;
}

// This is an example of a function that WONT be included in the search for CONSOLE_COMMAND
CONSOLE_COMMAND_2 
inline int print_hello_and_ret_3(int a, int b = 5)
{
    printf("hello! %d %d\n", a, b);
    return 3;
}

CONSOLE_COMMAND // Hello there
int my_function(int a);


namespace My_Namespace
{
void my_function2(int b);
}



class GOOD
{
public:
    static void my_function3(int a);
};