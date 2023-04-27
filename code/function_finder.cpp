#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>
#include <format>
#include <filesystem>
#include <functional>
#include <string_view>
#include <algorithm>

#include "function_finder/function_finder.hpp"

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
        return 0;
    }

    final_length += length;
    source = advance(source, length);

    source = skip_whitespace(source);

    out_arg.name = name;
    out_arg.type = type;
    
    // Check for default value
    if (source.size() > 0 && source[0] == '=')
    {
        out_arg.has_default_value = true;
        out_arg.default_value.type = type;
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

    source = skip_whitespace(source);

    // We got a comment!
    if (source[0] == '/')
    {
        if (source[1] == '/') // Line comment. Nice and easy.
        {

            std::string_view note_view = advance(source, (size_t)2);
            note_view = skip_whitespace(note_view);
            auto line_end = note_view.find('\n');
            out_function->note = std::string(note_view.begin(), note_view.begin() + line_end);
            ;
            source = advance(source, (size_t)source.find('\n') + 1);
        }
        else if ('*') // Block comment.
        {
        }
    }

    // Check for 'inline' modifier
    source = skip_whitespace(source);
    if(source.starts_with("inline"))
    {
        source = advance(source, sizeof("inline"));
    }

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

bool parse_file(const std::filesystem::path &path, std::vector<Function_Decl> &inout_commands)
{
    // Gather declarations
    std::ifstream file(path);
    if (!file.is_open())
    {
        printf("Couldn't find source file '%lls'!\n", path.c_str());
        return false;
    }

    std::stringstream ss;
    ss << file.rdbuf();
    auto content = ss.str();
    file.close();

    std::string_view file_view(content.begin(), content.end());
    size_t line = 1;
    int count = 0;

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
            func.line = line + 1;
            func.file = path.generic_string();
            try_parse_function(file_view, &func);
            inout_commands.push_back(func);
        }
    }

    return true;
}

bool write_output_file(const std::filesystem::path &path, std::vector<Function_Decl> &commands)
{
    std::ofstream file(path);
    if (!file.is_open())
    {
        printf("Couldn't create destination file '%lls'!\n", path.c_str());
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
    file << "#include \"function_finder/function_finder.hpp\"\n";
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
        file << "inline bool command_" << cmd.name << "(std::vector<std::string> &args, Value &out_result)\n";
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
                file << " = ";
                output_value_to_stream(file, arg.default_value);
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

        if (cmd.return_type == Value_Type::VOID)
        {
            // This function call code is copy pasted 3 times, time to func it.
            file << "    " << cmd.name << "(";
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
        else if (cmd.return_type == Value_Type::STRING)
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
             << "\"" << cmd.name << "\", " << type_to_string(cmd.return_type) << ",\n";
        file << "        // Arguments\n";
        file << "        {\n";
        for (size_t i = 0; i < cmd.arguments.size(); i++)
        {
            const auto &arg = cmd.arguments[i];
            file << "            Argument(";

            file << "\"" << arg.name << "\", ";
            file << type_to_string(arg.type);
            if (arg.has_default_value)
            {
                file << ", ";
                if (arg.default_value.type != Value_Type::STRING)
                {
                    file << "(" << type_to_cpp_type(arg.default_value.type) << ")";
                }
                output_value_to_stream(file, arg.default_value);
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
        file << "        " << (int)cmd.num_optional_args << ",\n";
        file << "        \"" << cmd.note << "\"\n";
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
        
    std::filesystem::path src_path = args[1];
    std::filesystem::path dst_path = args[2];

    if(!dst_path.has_extension())
    {
        printf("Dst must be a file!\n");
        return 2;
    }


    std::filesystem::create_directories(dst_path.parent_path());

    std::vector<Function_Decl> commands;
    if (std::filesystem::is_regular_file(src))
    {
        printf("Preprocessing '%s' into '%s'\n", src, dst);
        parse_file(src, commands);
    }
    else
    {
        // It's a directory
        const std::vector<std::string> accepted_extensions = {
            ".cpp", ".hpp", ".h", ".c", ".cxx"};

        std::function<void(const std::filesystem::path & path)> scan_directory;
        
        scan_directory = [&](const std::filesystem::path &path)
        {
            for (const auto &ele : std::filesystem::directory_iterator(path))
            {
                if (ele.is_regular_file())
                {
                    if (std::any_of(accepted_extensions.begin(), accepted_extensions.end(), [&](const std::string &ex)
                                 { return ele.path().extension() == ex; }))
                    {
                        parse_file(ele.path(), commands);
                    }
                }
                else if (ele.is_directory())
                {
                    scan_directory(ele.path());
                }
                else
                {
                    printf("Dunno what '%s' is...\n", (char*)ele.path().c_str());
                }
            }
        };

        scan_directory(src);
    }

    write_output_file(dst, commands);
}