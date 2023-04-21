#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <cctype>
#include <variant>

#include "common.hpp"

struct Argument
{
    std::string name;
    Supported_Type type = Supported_Type::UNKNOWN;
    Value default_value;
    bool has_default_value = false;
};

struct Function_Decl
{
    std::string name;
    Supported_Type return_type;
    std::vector<Argument> arguments;
    int num_required_args;
    int num_optional_args;
};

void advance(std::string_view &source, int count)
{
    source = source.substr(count);
}

void skip_whitespace(std::string_view &source)
{
    while (std::isspace(source[0]))
    {
        advance(source, 1);
    }
}

Supported_Type get_type(std::string_view &source)
{
    skip_whitespace(source);
    std::string str;
    int len = get_symbol(source.data(), str);
    if (!len)
    {
        printf("Failed to get type string from '%s'", source.data());
    }
    advance(source, len);

    Supported_Type result = Supported_Type::UNKNOWN;
    if (str == "double")
    {
        result = Supported_Type::DOUBLE;
    }
    if (str == "float")
    {
        result = Supported_Type::FLOAT;
    }
    if (str == "int")
    {
        result = Supported_Type::INTEGER;
    }
    else if (str == "std::string")
    {
        result = Supported_Type::STRING;
    }
    else if (str == "bool")
    {
        result = Supported_Type::BOOLEAN;
    }
    else if (str == "void")
    {
        result = Supported_Type::VOID;
    }

    return result;
}

bool get_argument(std::string_view source, Argument &out_arg)
{
    skip_whitespace(source);

    Supported_Type type = get_type(source);
    skip_whitespace(source);
    std::string name;
    int len = get_symbol(source.data(), name);
    if (!len)
    {
        printf("Failed to get argument name at position '%s'\n", source.data());
    }

    advance(source, len);

    skip_whitespace(source);

    out_arg.name = name;
    out_arg.type = type;

    // Check for default value
    if (source[0] == '=')
    {
        out_arg.has_default_value = true;
        advance(source, 1);
        skip_whitespace(source);
        switch (type)
        {
        case Supported_Type::BOOLEAN:
        {

            bool result;
            int len = get_bool(source.data(), result);
            if (len)
            {
                out_arg.default_value = result;
                advance(source, len);
            }
            else
            {
                return false;
            }
            break;
        }
        case Supported_Type::FLOAT:
        {
            double result;
            bool success = get_double(source.data(), result);
            if (success)
            {
                out_arg.default_value = (float)result;
            }
            else
            {
                return false;
            }
            break;
        }
        case Supported_Type::DOUBLE:
        {
            double result;
            bool success = get_double(source.data(), result);
            if (success)
            {
                out_arg.default_value = result;
            }
            else
            {
                return false;
            }
            break;
        }
        case Supported_Type::INTEGER:
        {

            int result;
            int len = get_int(source.data(), result);
            if (len)
            {
                out_arg.default_value = result;
            }
            else
            {
                return false;
            }
            break;
        }
        case Supported_Type::STRING:
        {
            std::string result;
            int len = get_quoted_string(source.data(), result);
            if (len)
            {
                out_arg.default_value = result;
            }
            else
            {
                printf("Failed to get string default value %s\n", source.data());
            }
            advance(source, len);
            break;
        }
        }
    }

    return true;
}

bool get_arguments(std::string_view &source, std::vector<Argument> &out_args)
{
    skip_whitespace(source);

    if (source[0] != '(')
    {
        return false;
    }
    advance(source, 1);

    // We should now be within the parenthesis
    skip_whitespace(source);

    // Loop over all args
    while (true)
    {
        bool found_end_parenthesis = false;
        int len = 0;

        // Delimit each argument
        while (true)
        {
            if (source[len] == ',')
            {
                len++;
                break;
            }

            if (source[len] == ')')
            {
                found_end_parenthesis = true;
                break;
            }

            len += 1;
        }

        if (len > 0)
        {
            Argument arg{};
            bool success = get_argument(source.substr(0, len), arg);

            if (!success)
            {
                return false;
            }

            out_args.push_back(arg);
        }

        if (found_end_parenthesis)
        {
            break;
        }
        advance(source, len);
    }

    return true;
}

bool try_parse_function(std::string_view &source, Function_Decl *out_function)
{
    // Skip the "CONSOLE_COMMAND" bit
    skip_whitespace(source);
    advance(source, (int)sizeof("CONSOLE_COMMAND"));
    out_function->return_type = get_type(source);
    skip_whitespace(source);

    int len = get_symbol(source.data(), out_function->name);
    if (!len)
    {
        printf("Failed to get function name '%s'\n", source.data());
        return false;
    }
    advance(source, len);

    bool success = get_arguments(source, out_function->arguments);
    if (!success)
    {
        printf("Failed to get arguments for function '%s'\n", out_function->name.c_str());
        return false;
    }

    out_function->num_optional_args = (int)std::count_if(out_function->arguments.begin(), out_function->arguments.end(), [](Argument &arg)
                                                         { return arg.has_default_value; });
    out_function->num_required_args = out_function->arguments.size() - out_function->num_optional_args;

#if 0
    std::cout << "Parsed function '" << out_function->name << "'" << std::endl;
    std::cout << "  Returns type " << (int)out_function->return_type << std::endl;
    std::cout << "  Args [" << out_function->arguments.size() << "]" << std::endl;

    for (const auto &arg : out_function->arguments)
    {
        std::cout << "    Name: " << arg.name << " - Type: " << (int)arg.type;

        if (arg.has_default_value)
        {
            std::cout << " - Default: ";

            const int *int_value = std::get_if<int>(&arg.default_value);
            if (int_value)
            {
                std::cout << *int_value;
            }
            const double *double_value = std::get_if<double>(&arg.default_value);
            if (double_value)
            {
                std::cout << *double_value;
            }
            const bool *bool_value = std::get_if<bool>(&arg.default_value);
            if (bool_value)
            {
                std::cout << *bool_value;
            }
            const std::string *string_value = std::get_if<std::string>(&arg.default_value);
            if (string_value)
            {
                std::cout << *string_value;
            }
        }

        std::cout << std::endl;
    }
#endif
    return true;
}

bool parse_file(const char *path, std::vector<Function_Decl> &inout_commands)
{
    // Gather declarations
    std::string line;
    std::ifstream file(path);
    if (!file.is_open())
    {
        printf("Couldn't find source file '%s'!\n", path);
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    auto content = ss.str();
    file.close();

    size_t pos = 0;
    int count = 0;

    while ((pos = content.find("\nCONSOLE_COMMAND", pos)) != std::string_view::npos)
    {
        std::string_view view(content.begin() + pos, content.end());
        Function_Decl func;
        bool success = try_parse_function(view, &func);

        inout_commands.push_back(func);
        pos += 1;
    }

    return true;
}

bool write_output_file(const char *path, std::vector<Function_Decl> &commands)
{
    std::ofstream file(path);
    if (!file.is_open())
    {
        printf("Couldn't create destination file file '%s'!\n", path);
        return false;
    }

    // Header
#pragma region Predecls
    file << "// This file is auto-generated. Don't edit!" << std::endl
         << std::endl;
    file << "#pragma once" << std::endl
         << std::endl;

#pragma region Includes
    // Includes
    file << "// Includes:\n";
    file << "#include <unordered_map>\n";
    file << "#include <string>\n";
    file << "#include \"common.hpp\"\n";
    file << std::endl
         << std::endl;

    file << "typedef bool (*Command_Wrapper)(std::vector<std::string> &args, Value &out_result);\n";
    file << "typedef std::unordered_map<std::string, Command_Wrapper> Command_Map;\n";

    file << std::endl;
#pragma endregion

    // Pre-definitions
#pragma region Predecls

    file << "// Pre-definitions:\n";
    for (const auto &cmd : commands)
    {
        file << supported_type_to_cpp_type(cmd.return_type) << " " << cmd.name << "(";
        for (int i = 0; i < cmd.arguments.size(); i++)
        {
            const Argument &arg = cmd.arguments[i];
            file << supported_type_to_cpp_type(arg.type) << " " << arg.name;

            // NOTE:
            //   We intentionally leave out the default value here. Since the compiler will complain
            //   if we define it twice. Default arguments are handled in the generated
            //   command_* functions placed below.                              @Daniel 10/04/2023

            if (i != cmd.arguments.size() - 1)
            {
                file << ", ";
            }
        }
        file << ");\n";
    }
    file << std::endl;
#pragma endregion

#pragma region wrappers

    // Write the command_* wrapper functions
    for (const auto &cmd : commands)
    {
        // Function definition
        file << "bool command_" << cmd.name << "(std::vector<std::string> &args, Value &out_result)\n";
        file << "{\n";

        // Confirm number of arguments
        if (cmd.num_required_args)
        {
            file << "    if(args.size() < " << cmd.num_required_args << ")\n";
            file << "    {\n";
            file << "        printf(\"Not enough arguments for '" << cmd.name << "'. Needed " << cmd.num_required_args << ", but got %zu\\n\", args.size());\n";
            file << "        return false;\n";
            file << "    }\n\n";
        }

        // Reused success flag for value parsing.
        if (cmd.arguments.size() > 0)
        {
            file << "    int success;\n\n";
        }

        // Add parsing and fetching to each argument.
        for (int i = 0; i < cmd.arguments.size(); i++)
        {
            const Argument &arg = cmd.arguments[i];

            // Header comment
            file << "    // Arg " << i << " '" << arg.name << "' with ";
            if (!arg.has_default_value)
            {
                file << "no default\n";
            }
            else
            {
                file << "default value ";
                output_value_to_stream(file, arg.default_value);
                file << "\n";
            }

            // Create the resulting variable
            file << "    " << supported_type_to_cpp_type(arg.type) << " arg_" << arg.name;
            if (arg.has_default_value)
            {
                file << " = \"";
                output_value_to_stream(file, arg.default_value);
                file << "\"";
            }
            file << ";\n";

            // If there's a default value, check if the value has been given. If it is given, parse it.
            if (arg.has_default_value)
            {
                file << "    if(args.size() > " << i << ")\n";
                file << "    {\n";

                // If it's a string, then we don't need parsing.
                if (arg.type == Supported_Type::STRING)
                {
                    file << "        success = true;\n";
                    file << "        arg_" << arg.name << " = args[" << i << "];\n";
                }
                else
                {
                    file << "        success = get_" << supported_type_to_readable_string(arg.type) << "(args[" << i << "].c_str(), arg_" << arg.name << ");\n";
                }
                file << "        if(!success)\n";
                file << "        {\n";
                file << "            printf(\"Failed to parse argument '" << arg.name << "' at index " << i << ". Expected " << supported_type_to_cpp_type(arg.type) << " but got string '%s'\\n\", args[" << i << "].c_str());\n";
                file << "            return false;\n";
                file << "        }\n";
                file << "    }\n";
            }
            else
            {
                if (arg.type == Supported_Type::STRING)
                {
                    file << "    success = true;\n";
                    file << "    arg_" << arg.name << " = args[" << i << "];\n";
                }
                else
                {
                    file << "    success = get_" << supported_type_to_readable_string(arg.type) << "(args[" << i << "].c_str(), arg_" << arg.name << ");\n";
                }
                file << "    if(!success)\n";
                file << "    {\n";
                file << "        printf(\"Failed to parse argument '" << arg.name << "' at index " << i << ". Expected " << supported_type_to_cpp_type(arg.type) << " but got string '%s'\\n\", args[" << i << "].c_str());\n";
                file << "        return false;\n";
                file << "    }\n";
                file << "\n";
            }
        }

        file << "    out_result = " << cmd.name << "(";

        for (int i = 0; i < cmd.arguments.size(); i++)
        {
            const Argument &arg = cmd.arguments[i];
            file << "arg_" << arg.name;

            if (i < cmd.arguments.size() - 1)
            {
                file << ", ";
            }
        }
        file << ");\n\n";
        file << "    return true;\n";
        file << "}\n\n";
    }

#pragma endregion

    // Write the 'init_commands' function
    file << "// Consumer:" << std::endl;
    file << "Command_Map init_commands()\n";
    file << "{\n";
    file << "    Command_Map results{};\n";
    file << "\n";

    for (const auto &cmd : commands)
    {
        file << "    results[\"" << cmd.name << "\"] = &command_" << cmd.name << ";\n";
    }
    file << "\n";
    file << "    return results;\n";
    file << "}" << std::endl;

    file.close();
    return true;
}

int main(int arg_count, const char **args)
{
    if (arg_count != 3)
    {
        printf("Needs a input path and output path as argument. Idiot\n");
        return 1;
    }
    const char *src = args[1];
    const char *dst = args[2];

    printf("Preprocessing '%s' into '%s'\n", src, dst);
    std::vector<Function_Decl> commands;
    parse_file(src, commands);

    write_output_file(dst, commands);
}