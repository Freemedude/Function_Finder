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

const char *skip_whitespace(const char *str)
{
    const char *cur = str;
    while (*cur == ' ' || *cur == '\t')
    {
        cur++;
    }
    return cur;
}

inline int get_int(const char *source, int &out_result)
{
    int len = 0;
    if (source[0] == '-' || source[0] == '+')
    {
        len++;
    }
    while (std::isdigit(source[len]))
    {
        len++;
    }

    out_result = std::atoi(source);

    return len;
}

inline int get_bool(const char *source, bool &out_result)
{
    int len = 0;
    if (strstr(source, "true") == source)
    {
        len = 4;
        out_result = true;
    }
    else if (strstr(source, "false") == source)
    {
        len = 5;
        out_result = false;
    }

    return len;
}

inline int get_quoted_string(const char *source, std::string &out_result)
{
    int len = 0;

    const char *cur = skip_whitespace(source);
    if (*cur != '\"')
    {
        printf("A QUOTED STRING MUST BE QUOTED! Expected '\"', but got %c\n", *cur);
        return 0;
    }
    len++;

    while (source[len] && source[len] != '\"')
    {
        len++;
    }

    if (source[len] != '\"')
    {
        // We must terminate on a quote if the string is quoted!
        return 0;
    }

    // Add the terminating quote into the length
    len++;

    out_result = std::string(source + 1, source + len - 1);

    return len;
}

inline int get_string(const char *source, std::string &out_result)
{
    int len = 0;

    bool is_quoted = source[len] == '\"';
    if(is_quoted)
    {
        len = get_quoted_string(source, out_result);
    }
    else
    {
        while (source[len] && !std::isspace(source[len]))
        {
            len++;
        }
        
        out_result = std::string(source, source + len);
    }

    return len;
}


inline int get_symbol(const char *source, std::string &out_result)
{
    int len = 0;
    while (std::isalnum(source[len]) || source[len] == '_' || source[len] == ':')
    {
        len++;
    }

    out_result = std::string(source, source + len);

    return len;
}

inline int get_double(const char *source, double &out_result)
{
    int len = 0;
    if (source[0] == '-' || source[0] == '+')
    {
        len++;
    }
    while (std::isdigit(source[len]))
    {
        len++;
    }

    // Handle comma separation
    if (source[len] == '.')
    {
        len++;
    }

    while (std::isdigit(source[len]))
    {
        len++;
    }

    if (len == 0)
    {
        return false;
    }

    out_result = std::atof(source);

    return len;
}
inline int get_float(const char *source, float &out_result)
{
    int len = 0;
    if (source[0] == '-' || source[0] == '+')
    {
        len++;
    }
    while (std::isdigit(source[len]))
    {
        len++;
    }

    // Handle comma separation
    if (source[len] == '.')
    {
        len++;
    }

    while (std::isdigit(source[len]))
    {
        len++;
    }

    if (len == 0)
    {
        return false;
    }

    // Handle trailing f
    if (source[len] == 'f')
    {
        len++;
    }

    double res = std::atof(source);
    out_result = (float)res;

    return len;
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