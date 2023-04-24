#pragma once

#define CONSOLE_COMMAND

#include <cstring>
#include <string>
#include <iostream>
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
    int int_value;
    float float_value;
    double double_value;
    bool bool_value;
    char string_value[128];
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

    Argument(const std::string &name, Value_Type type, std::string default_value)
        : Argument(name, type)
    {
        this->has_default_value = true;
        this->default_value.type = Value_Type::STRING;
        strncpy(this->default_value.data.string_value, default_value.data(), sizeof(this->default_value));
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

    Function_Decl() = default;
    Function_Decl(const std::string &file,
                  size_t line,
                  Command_Wrapper function,
                  std::string name,
                  Value_Type return_type,
                  std::vector<Argument> arguments,
                  int num_required_args,
                  int num_optional_args)
    {
        this->file = file;
        this->line = line;
        this->function = function;
        this->name = name;
        this->return_type = return_type;
        this->arguments = arguments;
        this->num_required_args = num_required_args;
        this->num_optional_args = num_optional_args;
    }
};

typedef std::unordered_map<std::string, Function_Decl> Command_Map;

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

inline void output_value_to_stream(std::ostream &stream, const Value &value)
{
    printf("TYPE: %d\n", value.type);
    switch (value.type)
    {
    case Value_Type::VOID:
        stream << "void";
        break;
    case Value_Type::STRING:
        stream << value.data.string_value;
        break;
    case Value_Type::INTEGER:
        stream << value.data.int_value;
        break;
    case Value_Type::FLOAT:
        stream << value.data.float_value << "f";
        break;
    case Value_Type::DOUBLE:
        stream << value.data.double_value;
        break;
    case Value_Type::BOOLEAN:
        stream << value.data.bool_value;
        break;
    default:
        break;
    }
}