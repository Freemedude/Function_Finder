#pragma once

#include <cstring>
#include <string>
#include <iostream>
#include <string_view>
#include <unordered_map>

enum class Value_Type
{
    UNKNOWN,
    VOID,
    STRING,
    INTEGER,
    FLOAT,
    DOUBLE,
    BOOLEAN
};

union Value_Data
{
    char string_value[128];
    int int_value;
    float float_value;
    double double_value;
    bool bool_value;
};

struct Value
{
    Value_Type type;
    Value_Data data;
};

struct Argument
{
    std::string name;
    Value_Type type = Value_Type::UNKNOWN;
    bool has_default_value = false;
    Value default_value;

    Argument() = default;
    Argument(const std::string &name, Value_Type type)
    {
        this->name = name;
        this->type = type;
    }

    Argument(const std::string &name, Value_Type type, const char *default_value)
        : Argument(name, type)
    {
        this->has_default_value = true;
        this->default_value.type = Value_Type::STRING;
        strncpy(this->default_value.data.string_value, default_value, sizeof(this->default_value));
    }

    Argument(const std::string &name, Value_Type type, int default_value)
        : Argument(name, type)
    {
        this->has_default_value = true;
        this->default_value.type = Value_Type::INTEGER;
        this->default_value.data.int_value = default_value;
    }

    Argument(const std::string &name, Value_Type type, float default_value)
        : Argument(name, type)
    {
        this->has_default_value = true;
        this->default_value.type = Value_Type::FLOAT;
        this->default_value.data.float_value = default_value;
    }

    Argument(const std::string &name, Value_Type type, double default_value)
        : Argument(name, type)
    {
        this->has_default_value = true;
        this->default_value.type = Value_Type::DOUBLE;
        this->default_value.data.double_value = default_value;
    }

    Argument(const std::string &name, Value_Type type, bool default_value)
        : Argument(name, type)
    {
        this->has_default_value = true;
        this->default_value.type = Value_Type::BOOLEAN;
        this->default_value.data.bool_value = default_value;
    }
};

typedef bool (*Command_Wrapper)(std::vector<std::string> &args, Value &out_result);

struct Function_Decl
{
    std::string file = "UNSET";
    size_t line = 0;
    Command_Wrapper function;
    std::string name;
    Value_Type return_type;
    std::vector<Argument> arguments;
    int num_required_args;
    int num_optional_args;
    std::string note;

    Function_Decl() = default;
    Function_Decl(const std::string &file,
                  size_t line,
                  Command_Wrapper function,
                  std::string name,
                  Value_Type return_type,
                  std::vector<Argument> arguments,
                  int num_required_args,
                  int num_optional_args,
                  const std::string &note)
    {
        this->file = file;
        this->line = line;
        this->function = function;
        this->name = name;
        this->return_type = return_type;
        this->arguments = arguments;
        this->num_required_args = num_required_args;
        this->num_optional_args = num_optional_args;
        this->note = note;
    }
};

typedef std::unordered_map<std::string, Function_Decl> Command_Map;

std::string_view advance(std::string_view source, size_t length)
{
    return source.substr(length);
}

std::string_view skip_whitespace(std::string_view source)
{
    size_t max_length = source.size();
    size_t length = 0;
    while (length < max_length && std::isspace(source[(int)length]))
    {
        length++;
    }
    return advance(source, length);
}

inline size_t get_int(std::string_view source, int &out_result)
{
    size_t max_length = source.size();
    size_t length = 0;
    if (length < max_length && source[0] == '-' || source[0] == '+')
    {
        length++;
    }
    while (length < max_length && std::isdigit(source[length]))
    {
        length++;
    }

    out_result = std::atoi(source.data());

    return length;
}

inline size_t get_bool(std::string_view source, bool &out_result)
{
    size_t length = 0;
    if (source.starts_with("true"))
    {
        length = 4;
        out_result = true;
    }
    else if (source.starts_with("false"))
    {
        length = 5;
        out_result = false;
    }
    if (source.starts_with("1"))
    {
        length = 1;
        out_result = true;
    }
    else if (source.starts_with("0"))
    {
        length = 1;
        out_result = false;
    }

    return length;
}

inline size_t get_quoted_string(std::string_view source, std::string &out_result)
{
    size_t length = 0;
    if (source[0] != '\"')
    {
        printf("[ERROR] get_quoted_string() takes a string that start with a quote (after whitespace)\n");
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
inline size_t get_word_length(std::string_view source)
{
    size_t length = 0;
    while (length < source.size() && source[length] && !std::isspace(source[length]))
    {
        length++;
    }

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
        length = get_word_length(source);

        out_result = std::string(source.begin(), source.begin() + length);
    }

    return length;
}

inline size_t get_symbol(std::string_view source, std::string &out_result)
{
    size_t max_length = source.size();
    size_t length = 0;
    while (length < max_length && (std::isalnum(source[length]) || source[length] == '_' || source[length] == ':'))
    {
        length++;
    }

    out_result = std::string(source.begin(), source.begin() + length);

    return length;
}

inline size_t get_double(std::string_view source, double &out_result)
{
    size_t max_length = source.size();
    size_t total_length = get_word_length(source);
    size_t length = 0;
    bool had_digits = false;

    if (length < max_length && (source[0] == '-' || source[0] == '+'))
    {
        length++;
    }

    while (length < max_length && std::isdigit(source[length]))
    {
        length++;
        had_digits = true;
    }

    // Handle comma separation
    if (length < max_length && source[length] == '.')
    {
        length++;
    }

    while (length < max_length && std::isdigit(source[length]))
    {
        length++;
        had_digits = true;
    }

    // We must have SOME digits. Either '.1' or '1.' is allowed, but not just '.'
    if (!had_digits)
    {
        return false;
    }

    if (length == 0 || length != total_length)
    {
        // We must have used all tokens here!
        return false;
    }

    out_result = std::atof(source.data());

    return length;
}

inline size_t get_float(std::string_view source, float &out_result)
{
    size_t max_length = source.size();
    size_t total_length = get_word_length(source);
    size_t length = 0;
    bool had_digits = false;

    if (length < max_length && (source[0] == '-' || source[0] == '+'))
    {
        length++;
    }

    while (length < max_length && std::isdigit(source[length]))
    {
        length++;
        had_digits = true;
    }

    // Handle comma separation
    if (length < max_length && source[length] == '.')
    {
        length++;
    }

    while (length < max_length && std::isdigit(source[length]))
    {
        length++;
        had_digits = true;
    }

    if (length < max_length && source[length] == 'f')
    {
        length++;
    }

    // We must have SOME digits. Either '.1' or '1.' is allowed, but not just '.'
    if (!had_digits)
    {
        return false;
    }

    if (length == 0 || length != total_length)
    {
        // We must have used all tokens here!
        return false;
    }

    double res = std::atof(source.data());
    out_result = (float)res;

    return length;
}

inline std::string type_to_cpp_type(Value_Type type)
{
    switch (type)
    {
    case Value_Type::VOID:
        return "void";
    case Value_Type::STRING:
        return "std::string";
    case Value_Type::INTEGER:
        return "int";
    case Value_Type::FLOAT:
        return "float";
    case Value_Type::DOUBLE:
        return "double";
    case Value_Type::BOOLEAN:
        return "bool";
    }
    return "";
}

inline std::string type_to_readable_string(Value_Type type)
{
    switch (type)
    {
    case Value_Type::VOID:
        return "void";
    case Value_Type::STRING:
        return "string";
    case Value_Type::INTEGER:
        return "int";
    case Value_Type::FLOAT:
        return "float";
    case Value_Type::DOUBLE:
        return "double";
    case Value_Type::BOOLEAN:
        return "bool";
    }
    return "";
}

inline std::string type_to_string(Value_Type type)
{
    switch (type)
    {
    case Value_Type::VOID:
        return "Value_Type::VOID";
    case Value_Type::STRING:
        return "Value_Type::STRING";
    case Value_Type::INTEGER:
        return "Value_Type::INTEGER";
    case Value_Type::FLOAT:
        return "Value_Type::FLOAT";
    case Value_Type::DOUBLE:
        return "Value_Type::DOUBLE";
    case Value_Type::BOOLEAN:
        return "Value_Type::BOOLEAN";
    }
    return "";
}

inline bool is_valid_float(std::string_view source)
{
    bool found_dot = false;

    for (int i = 0; i < source.size(); i++)
    {
    }
}

inline void output_value_to_stream(std::ostream &stream, const Value &value)
{
    switch (value.type)
    {
    case Value_Type::VOID:
        stream << "void";
        break;
    case Value_Type::STRING:
        stream << "\"" << value.data.string_value << "\"";
        break;
    case Value_Type::INTEGER:
        stream << value.data.int_value;
        break;
    case Value_Type::FLOAT:
        stream << std::to_string(value.data.float_value) << "f";
        break;
    case Value_Type::DOUBLE:
        stream << std::to_string(value.data.double_value);
        break;
    case Value_Type::BOOLEAN:
        stream << value.data.bool_value ? "true" : "false";
        break;
    default:
        break;
    }
}