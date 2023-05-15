// Example of usage code:
// For this example, run Function Finder with these arguments 'client.cpp output.hpp MY_COMMAND init_my_commands':
// client.cpp

#include <iostream>
#include <format>
#include "function_finder/function_finder.hpp"
#include "output.hpp"

#define MY_COMMAND

int main()
{
    Function_Map commands;
    init_my_commands(commands);

    // This is the variable we output all the commands to:
    Value value;

    std::cout << "Correct usage:\n";
    std::vector<std::string> args = {"5", "1.2f", "42.0", "Not using the default!", "false"};
    std::cout << "    ";
    commands["some_command"].function(args, value);
    std::cout << "    " << to_string(value) << "\n";
    std::cout << "\n";

    std::cout << "Correct usage using defaults:\n";
    args = {"5", "1.2f", "42.0"};
    std::cout << "    ";
    commands["some_command"].function(args, value);
    std::cout << "    " << to_string(value) << "\n";
    std::cout << "\n";

    std::cout << "Incorrect usage, wrong type:\n"; 
    args = {"5", "Woups! A string doesn't go here!", "5"}; 
    std::cout << "    "; 
    commands["some_command"].function(args, value); 
    std::cout << "\n";

    std::cout << "Incorrect usage, wrong number of arguments:\n";
    args = {"5"};
    std::cout << "    ";
    commands["some_command"].function(args, value);
    std::cout << "\n";

    // To make it easier to integrate with auto-complete and whatever else you'd want, the Function_Map includes much more information about the functions:
    std::cout << "To make it easier to integrate with auto-complete and whatever else you'd want, the Function_Map includes much more information about the functions:\n";
    Function_Decl &command = commands["some_command"];
    std::cout << " - File: " << command.file << "\n";
    std::cout << " - Line: " << command.line << "\n";
    std::cout << " - Name: " << command.name << "\n";
    std::cout << " - Note (The comment after the search term): " << command.note << "\n";
    std::cout << " - Return type: " << to_string(command.return_type) << "\n";
    std::cout << " - Argument information (types, names, and default arguments.):\\n";
    for(const auto &arg : command.arguments)
    {
        if(arg.has_default_value)
        {
            std::cout << std::format("    - {} {} = {}\n", 
                value_type_to_cpp_type(arg.type), arg.name, to_string(arg.default_value));
        }
        else
        {
            std::cout << std::format("    - {} {}\n", value_type_to_cpp_type(arg.type), arg.name);
        }
    }

    return 0;
}

// This is the command we want it to find!
MY_COMMAND // I can even give it a comment to act as documentation!
bool some_command(int i, float f, double d, std::string s = "default_values_are_supported", bool b = false)
{
    std::cout << std::format("Hello from 'some_command'! These were the arguments: i = {}, f = {}, d = {}, s = {}, b = {}\n", i, f, d, s, b);
    std::cout << "Returning if i == 5!\n";
    return i == 5;
}

// This is the command we want it to find!
MY_COMMAND 
/*
    This function is parsed, but not used for anything.
*/
int some_other_command(int integer)
{
    return integer + 2;
}