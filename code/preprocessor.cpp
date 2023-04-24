#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <cctype>
#include <format>

#include "common.hpp"

size_t get_type(std::string_view source, Value_Type &out_type)
{
    size_t result = 0;
    source = skip_whitespace(source);
    std::string str;
    size_t length = get_symbol(source, str);
    if (!length)
    {
        printf("Failed to get type string from '%s'", source.data());
        out_type = Value_Type::UNKNOWN;
        return 0;
    }

    result += length;
    source = advance(source, length);

    Value_Type type = Value_Type::UNKNOWN;
    if (str == "double")
    {
        type = Value_Type::DOUBLE;
    }
    if (str == "float")
    {
        type = Value_Type::FLOAT;
    }
    if (str == "int")
    {
        type = Value_Type::INTEGER;
    }
    else if (str == "std::string")
    {
        type = Value_Type::STRING;
    }
    else if (str == "bool")
    {
        type = Value_Type::BOOLEAN;
    }
    else if (str == "void")
    {
        type = Value_Type::VOID;
    }

    out_type = type;

    return result;
}

size_t get_argument(std::string_view source, Argument &out_arg)
{
    source = skip_whitespace(source);
    Value_Type type;
    size_t final_length = get_type(source, type);
    source = advance(source, final_length);
    source = skip_whitespace(source);
    std::string name;
    size_t length = get_symbol(source, name);
    if (!length)
    {
        printf("Failed to get argument name at position '%s'\n", source.data());
    }

    final_length += length;
    source = advance(source, length);

    source = skip_whitespace(source);

    out_arg.name = name;
    out_arg.type = type;

    // Check for default value
    if (source[0] == '=')
    {
        out_arg.has_default_value = true;
        out_arg.default_value.type = type;
        printf("Default value %s has type %d\n", name.c_str(), (int)type);
        source = advance(source, (size_t)1);
        source = skip_whitespace(source);
        switch (type)
        {
        case Value_Type::BOOLEAN:
        {

            bool result;
            size_t length = get_bool(source, result);
            if (length)
            {
                out_arg.default_value.data.bool_value = result;
                source = advance(source, length);
                final_length += length;
            }
            else
            {
                return false;
            }
            break;
        }
        case Value_Type::FLOAT:
        {
            float result;
            size_t length = get_float(source, result);
            if (length)
            {
                out_arg.default_value.data.float_value = result;
                source = advance(source, length);
                final_length += length;
            }
            else
            {
                return 0;
            }
            break;
        }
        case Value_Type::DOUBLE:
        {
            double result;
            size_t length = get_double(source, result);
            if (length)
            {
                out_arg.default_value.data.double_value = result;
                source = advance(source, length);
                final_length += length;
            }
            else
            {
                return 0;
            }
            break;
        }
        case Value_Type::INTEGER:
        {

            int result;
            size_t length = get_int(source, result);
            if (length)
            {
                out_arg.default_value.data.int_value = result;
                source = advance(source, length);
                final_length += length;
            }
            else
            {
                return false;
            }
            break;
        }
        case Value_Type::STRING:
        {
            std::string result;
            size_t length = get_quoted_string(source, result);
            if (length)
            {
                strncpy(out_arg.default_value.data.string_value, result.c_str(), result.length());
                source = advance(source, length);
                final_length += length;
            }
            else
            {
                printf("Failed to get string default value %s\n", source.data());
                return 0;
            }
            break;
        }
        }
    }

    return final_length;
}

bool get_arguments(std::string_view source, std::vector<Argument> &out_args)
{
    source = skip_whitespace(source);

    if (source[0] != '(')
    {
        return false;
    }
    source = advance(source, (size_t)1);

    // We should now be within the parenthesis
    source = skip_whitespace(source);

    // Loop over all args
    while (true)
    {
        bool found_end_parenthesis = false;
        size_t length = 0;

        // Delimit each argument
        while (true)
        {
            if (source[length] == ',')
            {
                length++;
                break;
            }

            if (source[length] == ')')
            {
                found_end_parenthesis = true;
                break;
            }

            length += 1;
        }

        if (length > 0)
        {
            Argument arg{};
            bool success = get_argument(source.substr(0, length), arg);

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
        source = advance(source, length);
    }

    return true;
}

size_t try_parse_function(std::string_view source, Function_Decl *out_function)
{
    // Skip the "CONSOLE_COMMAND" bit
    source = skip_whitespace(source);
    source = advance(source, sizeof("CONSOLE_COMMAND"));
    size_t final_length = get_type(source, out_function->return_type);
    source = advance(source, final_length);
    source = skip_whitespace(source);

    size_t length = get_symbol(source, out_function->name);
    if (!length)
    {
        printf("Failed to get function name '%s'\n", source.data());
        return false;
    }

    final_length += length;
    source = advance(source, length);

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
    return final_length;
}

bool parse_file(const char *path, std::vector<Function_Decl> &inout_commands)
{
    // Gather declarations
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

    std::string_view file_view(content.begin(), content.end());
    size_t line = 1;
    int count = 0;
#if 1
    while (true)
    {
        auto next_line_end = file_view.find('\n');
        if (next_line_end == std::string_view::npos)
        {
            break;
        }

        file_view = file_view.substr(next_line_end + 1);
        line++;

        if (file_view.starts_with("CONSOLE_COMMAND"))
        {
            Function_Decl func;
            try_parse_function(file_view, &func);
            func.line = line + 1;
            func.file = std::string(path);
            inout_commands.push_back(func);
        }
    }

#else
    size_t pos = 0;
    while ((pos = content.find("\nCONSOLE_COMMAND", pos)) != std::string_view::npos)
    {
        std::string_view view(content.begin() + pos, content.end());
        Function_Decl func;
        size_t success = try_parse_function(view, &func);

        inout_commands.push_back(func);
        pos += 1;
    }
#endif
    return true;
}

bool write_output_file(const char *path, std::vector<Function_Decl> &commands)
{
    std::ofstream file(path);
    if (!file.is_open())
    {
        printf("Couldn't create destination file '%s'!\n", path);
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

    file << std::endl;
#pragma endregion

    // Pre-definitions
#pragma region Predecls

    file << "// Pre-definitions:\n";
    for (const auto &cmd : commands)
    {
        file << type_to_cpp_type(cmd.return_type) << " " << cmd.name << "(";
        for (int i = 0; i < cmd.arguments.size(); i++)
        {
            const Argument &arg = cmd.arguments[i];
            file << type_to_cpp_type(arg.type) << " " << arg.name;

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
        file << "// Generated based on function \"" << cmd.name << "\" from file \"" << cmd.file << "\" line " << cmd.line << "\n";
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
            file << "    " << type_to_cpp_type(arg.type) << " arg_" << arg.name;
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
                if (arg.type == Value_Type::STRING)
                {
                    file << "        success = true;\n";
                    file << "        arg_" << arg.name << " = args[" << i << "];\n";
                }
                else
                {
                    file << "        success = get_" << type_to_readable_string(arg.type) << "(args[" << i << "], arg_" << arg.name << ");\n";
                }
                file << "        if(!success)\n";
                file << "        {\n";
                file << "            printf(\"Failed to parse argument '" << arg.name << "' at index " << i << ". Expected " << type_to_cpp_type(arg.type) << " but got string '%s'\\n\", args[" << i << "].c_str());\n";
                file << "            return false;\n";
                file << "        }\n";
                file << "    }\n";
            }
            else
            {
                if (arg.type == Value_Type::STRING)
                {
                    file << "    success = true;\n";
                    file << "    arg_" << arg.name << " = args[" << i << "];\n";
                }
                else
                {
                    file << "    success = get_" << type_to_readable_string(arg.type) << "(args[" << i << "], arg_" << arg.name << ");\n";
                }
                file << "    if(!success)\n";
                file << "    {\n";
                file << "        printf(\"Failed to parse argument '" << arg.name << "' at index " << i << ". Expected " << type_to_cpp_type(arg.type) << " but got string '%s'\\n\", args[" << i << "].c_str());\n";
                file << "        return false;\n";
                file << "    }\n";
                file << "\n";
            }
        }
        file << "    out_result.type = " << type_to_string(cmd.return_type) << ";\n";

        if (cmd.return_type == Value_Type::STRING)
        {

            file << "    std::string result = " << cmd.name << "(";
            for (int i = 0; i < cmd.arguments.size(); i++)
            {
                const Argument &arg = cmd.arguments[i];
                file << "arg_" << arg.name;

                if (i < cmd.arguments.size() - 1)
                {
                    file << ", ";
                }
            }
            file << ");\n";
            file << "    strncpy(out_result.data.string_value, result.data(), sizeof(out_result.data.string_value));\n\n";
        }
        else
        {
            file << "    out_result.data." << type_to_readable_string(cmd.return_type) << "_value = " << cmd.name << "(";
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
        }

        file << "    return true;\n";
        file << "}\n\n";
    }

#pragma endregion

    // Write the 'init_commands' function
    file << "// Consumer:" << std::endl;
    file << "void init_commands(Command_Map &out_commands)\n";
    file << "{\n";

    for (const auto &cmd : commands)
    {
        file << "    out_commands[\"" << cmd.name << "\"] = Function_Decl(";
        file << "\"" << cmd.file << "\", (size_t)" << cmd.line << ", "
             << "&command_" << cmd.name << ", "
             << "\"" << cmd.name << "\", (Value_Type)" << (int)cmd.return_type << ",\n";
        file << "        // Arguments\n";
        file << "        {\n";
        for (size_t i = 0; i < cmd.arguments.size(); i++)
        {
            const auto &arg = cmd.arguments[i];
            file << "            // Argument " << arg.name << std::endl;
            file << "            Argument(";

            file << "\"" << arg.name << "\", ";
            file << "(Value_Type)" << (int)arg.type;
            if (arg.has_default_value)
            {
                file << ", ";
                file << "\"";
                output_value_to_stream(file, arg.default_value);
                file << "\"";
            }

            file << ")";
            if (i != cmd.arguments.size() - 1)
            {
                file << ",";
            }
            file << "\n";
        }
        file << "        },\n";
        file << "        " << (int)cmd.num_required_args << ",\n";
        file << "        " << (int)cmd.num_optional_args << "\n";
        file << "    );\n";
    }
    file << "\n";
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