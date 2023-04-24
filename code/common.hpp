#pragma once

#define CONSOLE_COMMAND

#include <cstring>
#include <iostream>
#include <variant>

using Value = std::variant<int, float, double, bool, std::string>;

enum class Supported_Type
{
    UNKNOWN,
    VOID,
    STRING,
    INTEGER,
    FLOAT,
    DOUBLE,
    BOOLEAN
};

std::string_view advance(std::string_view source, size_t count)
{
    return source.substr(count);
}

std::string_view skip_whitespace(std::string_view source)
{
    size_t count = 0;
    while (std::isspace(source[(int)count]))
    {
        count++;
    }
    return advance(source, count);
}

inline size_t get_int(std::string_view source, int &out_result)
{
    size_t length = 0;
    if (source[0] == '-' || source[0] == '+')
    {
        length++;
    }
    while (std::isdigit(source[length]))
    {
        length++;
    }

    out_result = std::atoi(source.data());

    return length;
}

inline size_t get_bool(std::string_view source, bool &out_result)
{
    size_t length = 0;
    if (strstr(source.data(), "true") == source)
    {
        length = 4;
        out_result = true;
    }
    else if (strstr(source.data(), "false") == source)
    {
        length = 5;
        out_result = false;
    }

    return length;
}

inline size_t get_quoted_string(std::string_view source, std::string &out_result)
{
    size_t length = 0;

    if (source[0] != '\"')
    {
        printf("[ERROR] get_quoted_string() takes a string that  start with a quote (after whitespace)\n");
        return 0;
    }
    length++;

    while (source[length] && source[length] != '\"')
    {
        length++;
    }

    if (source[length] != '\"')
    {
        // We must terminate on a quote if the string is quoted!
        return 0;
    }

    // Add the terminating quote into the length
    length++;

    out_result = std::string(source.begin() + 1, source.begin() + length - 1);

    return length;
}

inline size_t get_string(std::string_view source, std::string &out_result)
{
    size_t length = 0;

    bool is_quoted = source[length] == '\"';
    if (is_quoted)
    {
        length = get_quoted_string(source, out_result);
    }
    else
    {
        while (source[length] && !std::isspace(source[length]))
        {
            length++;
        }

        out_result = std::string(source.begin(), source.begin() + length);
    }

    return length;
}

inline size_t get_symbol(std::string_view source, std::string &out_result)
{
    size_t length = 0;
    while (std::isalnum(source[length]) || source[length] == '_' || source[length] == ':')
    {
        length++;
    }

    out_result = std::string(source.begin(), source.begin() + length);

    return length;
}

inline size_t get_double(std::string_view source, double &out_result)
{
    size_t length = 0;
    if (source[0] == '-' || source[0] == '+')
    {
        length++;
    }
    while (std::isdigit(source[length]))
    {
        length++;
    }

    // Handle comma separation
    if (source[length] == '.')
    {
        length++;
    }

    while (std::isdigit(source[length]))
    {
        length++;
    }

    if (length == 0)
    {
        return false;
    }

    out_result = std::atof(source.data());

    return length;
}

inline size_t get_float(std::string_view source, float &out_result)
{
    size_t length = 0;
    if (source[0] == '-' || source[0] == '+')
    {
        length++;
    }
    while (std::isdigit(source[length]))
    {
        length++;
    }

    // Handle comma separation
    if (source[length] == '.')
    {
        length++;
    }

    while (std::isdigit(source[length]))
    {
        length++;
    }

    if (length == 0)
    {
        return false;
    }

    // Handle trailing f
    if (source[length] == 'f')
    {
        length++;
    }

    double res = std::atof(source.data());
    out_result = (float)res;

    return length;
}

inline std::string supported_type_to_cpp_type(Supported_Type type)
{
    switch (type)
    {
    case Supported_Type::VOID:
        return "void";
    case Supported_Type::STRING:
        return "std::string";
    case Supported_Type::INTEGER:
        return "int";
    case Supported_Type::FLOAT:
        return "float";
    case Supported_Type::DOUBLE:
        return "double";
    case Supported_Type::BOOLEAN:
        return "bool";
    }
    return "";
}

inline std::string supported_type_to_readable_string(Supported_Type type)
{
    switch (type)
    {
    case Supported_Type::VOID:
        return "void";
    case Supported_Type::STRING:
        return "string";
    case Supported_Type::INTEGER:
        return "int";
    case Supported_Type::FLOAT:
        return "float";
    case Supported_Type::DOUBLE:
        return "double";
    case Supported_Type::BOOLEAN:
        return "bool";
    }
    return "";
}

inline void output_value_to_stream(std::ostream &stream, const Value &value)
{
    {

        const int *int_value = std::get_if<int>(&value);
        if (int_value)
        {
            stream << *int_value;
        }
    }
    {
        const double *double_value = std::get_if<double>(&value);
        if (double_value)
        {
            stream << *double_value;
        }
    }
    {
        const float *float_value = std::get_if<float>(&value);
        if (float_value)
        {
            stream << *float_value << 'f';
        }
    }
    {

        const bool *bool_value = std::get_if<bool>(&value);
        if (bool_value)
        {
            stream << *bool_value;
        }
    }
    {

        const std::string *string_value = std::get_if<std::string>(&value);
        if (string_value)
        {
            stream << *string_value;
        }
    }
}