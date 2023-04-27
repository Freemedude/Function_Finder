#pragma once

#include "function_finder/function_finder.hpp"

CONSOLE_COMMAND
inline int print_hello_and_ret_2()
{
    printf("hello!\n");
    return 2;
}