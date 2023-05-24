# Function Finder

Function Finder is a C++20 command line tool used for locating and registering functions in other 
source files. It's meant to act as a work-around for C++'s lacking meta-programming features.
It scans a file or directory for all functions prefixed with a configurable search term, and exports
wrapper functions that parse the arguments as a string list {BAD PHRASING}. 
The expected use case is to make it easy to register functions for things like command consoles or
UI menus, which often requires a lot of manual labor to work. With this tool you can simply prefix
a function with eg. 'CONSOLE_COMMAND'; run function_finder as part of your build process; and it will
take care of argument parsing, gather debug information and forwarding of documentation!


Minimalist example:
```cpp
// ...

void user()
{
    auto commands = init_commands();

    auto arguments = split_arguments("5 \"My argument\"");

    // Whether or not to actually call. Useful for confirming that it SHOULD work before actually 
    //calling
    bool call_client_function = true;
    Call_Result result = commands["my_command"].function(arguments, call_client_function);
    if(result.status = Call_Result_Status::SUCCESS)
    {
        // It succeeded! Do something with the result!
    }
    else
    {
        // It failed! Print the message, or inspect the result to see why!
        std::cout << "Call to 'my_command' failed: " << result.error_message << '\n';


    }
}

CONSOLE_COMMAND /* This is the documentation for the command!

It can be multi-line, but you may have to experiment with it to fit how you want to display it! */
void my_command(int number, std::string argument, bool my_default = 5 /* Arguments can also have documentation! */)
{
    // ...
}
// ...
```

Basic usage is `function_finder <input> <output> <search_term> <init_function_name> <wrapper_function_prefix>`.



## Usage

## Requirements

## Documentation
