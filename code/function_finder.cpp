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
#include <regex>
#include <chrono>
#include <algorithm>

#include "function_finder/function_finder.hpp"

const int VERSION = 0;

const std::vector<std::string> ACCEPTED_EXTENSIONS = {
	".cpp", ".hpp", ".h", ".c", ".cxx" };

struct Settings
{
	std::filesystem::path source;
	std::filesystem::path destination;
	std::string search_term;
	std::string init_function_name;
};

size_t get_type(std::string_view source, Value_Type &out_type)
{
	size_t result = 0;
	source = skip_whitespace(source);
	std::string str;
	size_t length = get_symbol(source, str);
	if (!length)
	{
		std::cout << "Failed to get type string from '" << source << "'\n";
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
		std::cout << std::format("Failed to get argument name at position '{}'\n", source.data());
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
				std::cout << "Failed to get string default value %s" << source.data() << "\n";
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
	// We got a comment!
	if (source[0] == '/')
	{
		if (source[1] == '/') // Line comment. Nice and easy.
		{

			std::string_view note_view = advance(source, (size_t)2);
			note_view = skip_whitespace(note_view);
			auto line_end = note_view.find('\n');
			out_function->note = std::string(note_view.begin(), note_view.begin() + line_end);
			source = advance(source, (size_t)source.find('\n') + 1);
		}
		else if (source[1] == '*') // Block comment.
		{
			// Find the end of the comment.
			auto comment_end = source.find("*/");
			if (comment_end == std::string_view::npos)
			{
				// There is no end to this block comment. It will consume us all.
				return 0;
			}

			std::string_view note_view(source.begin() + 2, source.begin() + comment_end);
			note_view = skip_whitespace(note_view);
			out_function->note = std::string(note_view.begin(), note_view.end());

			// Sanitize.
			out_function->note = std::regex_replace(out_function->note, std::regex("\n"), "\\n");
			source = advance(source, comment_end + 2); // +2 to skip the '*/' bit
		}
	}

	// Check for 'inline' modifier
	source = skip_whitespace(source);
	if (source.starts_with("inline"))
	{
		source = advance(source, sizeof("inline"));
	}

	size_t final_length = get_type(source, out_function->return_type);
	source = advance(source, final_length);
	source = skip_whitespace(source);

	size_t length = get_symbol(source, out_function->name);
	if (!length)
	{
		std::cerr << "[ERROR] Failed to get function name '" << source << "'\n";
		return false;
	}

	final_length += length;
	source = advance(source, length);

	bool success = get_arguments(source, out_function->arguments);
	if (!success)
	{
		std::cerr << "[ERROR] Failed to get arguments for function '" << out_function->name << "'\n";
		return false;
	}

	out_function->num_optional_args = (int)std::count_if(out_function->arguments.begin(), out_function->arguments.end(), [](Argument &arg)
		{ return arg.has_default_value; });
	out_function->num_required_args = out_function->arguments.size() - out_function->num_optional_args;

	return final_length;
}

bool parse_file(const std::filesystem::path &path, std::vector<Function_Decl> &inout_functions, std::string_view search_term)
{
	// Gather declarations
	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cerr << "[ERROR] Couldn't find source file '" << path << "'! Terminating." << std::endl;
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

		if (file_view.starts_with(search_term))
		{


			Function_Decl func;
			func.line = line + 1;
			func.file = path.generic_string();

			// Skip the search term
			auto func_view = skip_whitespace(file_view);
			func_view = advance(func_view, search_term.size());
			func_view = skip_whitespace(func_view);

			try_parse_function(func_view, &func);
			inout_functions.push_back(func);
		}
	}

	return true;
}

class Cpp_File_Writer
{
	int current_indent = 0;

	public:
	const int tab_size = 4;
	std::ostream &stream;

	explicit Cpp_File_Writer(std::ostream &stream)
		:stream(stream)
	{
	}

	template <typename T>
	std::ostream &operator<<(T const &obj)
	{
		for (int i = 0; i < current_indent * tab_size; i++)
		{
			stream << ' ';
		}

		stream << obj;

		stream << std::endl;
		return stream;
	}

	void indent()
	{
		current_indent++;

	}

	void unindent()
	{
		current_indent--;

	}

	void skip_line()
	{
		stream << std::endl;
	}

	std::ostream &get_stream()
	{
		for (int i = 0; i < current_indent * tab_size; i++)
		{
			stream << ' ';
		}

		return stream;

	}
};

std::string function_call_string(const Function_Decl &func)
{
	std::stringstream ss;
	ss << func.name << "(";
	for (int i = 0; i < func.arguments.size(); i++)
	{
		const Argument &arg = func.arguments[i];
		ss << "arg_" << arg.name;

		if (i < func.arguments.size() - 1)
		{
			ss << ", ";
		}
	}
	ss << ")";
	return ss.str();
}

bool export_functions(const Settings &settings, std::vector<Function_Decl> &functions)
{
	std::filesystem::create_directories(settings.destination.parent_path());

	std::ofstream file(settings.destination);

	if (!file.is_open())
	{
		std::cerr << "[ERROR] Could not create destination file at '" << settings.destination.generic_string() << "'! Terminating." << std::endl;
		return false;
	}

	Cpp_File_Writer w(file);

	// Header
	w << "// The contents of this file are auto-generated.";
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	w << std::format("// This file was generated {}", std::ctime(&now));
	w << "// Generation options:";
	w << std::format("//   Input path: {}", settings.source.generic_string());
	w << std::format("//   Output path: {}", settings.destination.generic_string());
	w << std::format("//   Search pattern: {}", settings.search_term);
	w << std::format("//   Initialization function name: {}", settings.init_function_name);
	w << "#pragma once";
	w.skip_line();

	// Includes
	w << "// Includes:";
	w << "#include <unordered_map>";
	w << "#include <string>";
	w << R"(#include "function_finder/function_finder.hpp")";
	w.skip_line();

#pragma region Predecls
	// Pre-declarations

	w << "//////////////////////////////";
	w << "//     PRE-DECLARATIONS     //";
	w << "//////////////////////////////";

	w << "// Pre-declarations";
	w << "// Default values are ommitted because it: 1) wont compile with them defined twice; 2) they are handled by the generated wrappers below.";
	w.skip_line();
	for (const auto &cmd : functions)
	{
		w << std::format("// From \"{}\" L{}", cmd.file, cmd.line);
		auto &s = w.get_stream() << type_to_cpp_type(cmd.return_type) << " " << cmd.name << "(";
		for (int i = 0; i < cmd.arguments.size(); i++)
		{
			const Argument &arg = cmd.arguments[i];
			s << type_to_cpp_type(arg.type) << " " << arg.name;

			// NOTE:
			//   We intentionally leave out the default value here. Since the compiler will complain
			//   if we define it twice. Default arguments are handled in the generated
			//   command_* functions placed below.                              @Daniel 10/04/2023
			if (i != cmd.arguments.size() - 1)
			{
				s << ", ";
			}
		}
		s << ");" << std::endl;
		w.skip_line();

	}
#pragma endregion

#pragma region wrappers

	w << "//////////////////////////////";
	w << "//         WRAPPERS         //";
	w << "//////////////////////////////";
	w.skip_line();

	// Write the command_* wrapper functions
	for (const auto &cmd : functions)
	{
		// Function definition
		w << std::format("// Generated based on function \"{}\" from file \"{}\" L{}", cmd.name, cmd.file, cmd.line);
		w << std::format("inline bool command_{}(std::vector<std::string> &args, Value &out_result)", cmd.name);
		w << "{";
		w.indent();

		// Confirm number of arguments
		if (cmd.num_required_args)
		{
			w << "// Check that all required arguments are provided.";
			w << std::format("if(args.size() < {})", cmd.num_required_args);
			w << "{";
			w.indent();

			w << std::format("printf(\"Not enough arguments for '{}'. Needed {}, but got %zu\\n\", args.size());", cmd.name, cmd.num_required_args);
			w << "return false;";

			w.unindent();
			w << "}";
			w.skip_line();
		}

		// Reused success flag for value parsing.
		if (cmd.arguments.size() > 0)
		{
			w << "// Reused success flag used for value parsing";
			w << "int success = false;";
			w.skip_line();
		}

		// Add parsing and fetching to each argument.
		for (int i = 0; i < cmd.arguments.size(); i++)
		{
			const Argument &arg = cmd.arguments[i];

			// Create variable
			w << std::format("// {} argument {}: '{} {}'", arg.has_default_value ? "Optional" : "Required", i, type_to_cpp_type(arg.type), arg.name);

			if (arg.has_default_value)
			{
				w << std::format("{} arg_{} = {};", type_to_cpp_type(arg.type), arg.name, value_to_string(arg.default_value));
				
				// Check if replacement variable has been provided
				w << std::format("if(args.size() > {})", i);
				w << "{";
				w.indent();

				// If it's a string, then we don't need parsing.
				if (arg.type == Value_Type::STRING)
				{
					w << "success = true;";
					w << std::format("arg_{} = args[{}];", arg.name, i);
				}
				else
				{
					w << std::format("success = get_{}(args[{}], arg_{});", type_to_readable_string(arg.type), i, arg.name);
				}
			}
			else
			{
				w << std::format("{} arg_{};", type_to_cpp_type(arg.type), arg.name);
				if (arg.type == Value_Type::STRING)
				{
					w << "success = true;";
					w << std::format("arg_{} = args[{}];", arg.name, i);
				}
				else
				{
					w << std::format("success = get_{}(args[{}], arg_{});", type_to_readable_string(arg.type), i, arg.name);
				}
			}

			w << "if(!success)";
			w << "{";
			w.indent();

			w << std::format("printf(\"Failed to parse argument {0} '{1}'. Attempted to parse a {2}, but got string '%s'\\n\", args[{0}].c_str());", i, arg.name, type_to_cpp_type(arg.type));
			w << "return 0;";

			w.unindent();
			w << "}";

			if (arg.has_default_value)
			{
				w.unindent();
				w << "}";
			}
			w.skip_line();
		}
		w << std::format("out_result.type = {};", type_to_string(cmd.return_type));

		if (cmd.return_type == Value_Type::VOID)
		{
			// This function call code is copy pasted 3 times, time to func it.
			w << std::format("{};", function_call_string(cmd));

		}
		else if (cmd.return_type == Value_Type::STRING)
		{
			w << std::format("std::string result = {};", function_call_string(cmd));
			w << "strncpy(out_result.data.string_value, result.data(), sizeof(out_result.data.string_value));";
		}
		else
		{
			w << std::format("out_result.data.{}_value = {};", type_to_readable_string(cmd.return_type), function_call_string(cmd));
		}
		w.skip_line();

		w << "return true;";

		w.unindent();
		w << "}";
		w.skip_line();
	}

#pragma endregion

	// Write the initialization function

	w << "//////////////////////////////";
	w << "//       INITIALIZER        //";
	w << "//////////////////////////////";
	w.skip_line();

	w << std::format("void {}(Function_Map &out_commands)", settings.init_function_name);
	w << "{";
	w.indent();

	for (const auto &cmd : functions)
	{
		w << std::format("out_commands[\"{0}\"] = Function_Decl(\"{0}\", command_{0}, {1}, {2}, {3},",
			cmd.name, type_to_string(cmd.return_type), (int)cmd.num_required_args, (int)cmd.num_optional_args);
		w.indent();
		w << std::format("\"{}\", (size_t){},", cmd.file, cmd.line);
		w << "// Arguments";
		w << "{";
		w.indent();
		for (size_t i = 0; i < cmd.arguments.size(); i++)
		{
			const auto &arg = cmd.arguments[i];
			bool last_iter = i == cmd.arguments.size() - 1;

			if (arg.has_default_value)
			{
				w << std::format("Argument(\"{}\", {}, {}){}",
					arg.name, type_to_string(arg.type), value_to_string(arg.default_value), last_iter ? "" : ",");
			}
			else
			{
				w << std::format("Argument(\"{}\", {}){}",
					arg.name, type_to_string(arg.type), last_iter ? "" : ",");
			}
		}
		w.unindent();
		w << "},";
		w << std::format("\"{}\"", cmd.note);
		w.unindent();
		w << ");";
		w.skip_line();
	}

	w.unindent();
	w << "}";
	w.skip_line();

	file.close();
	return true;
}



bool import_functions(const std::filesystem::path &path, std::vector<Function_Decl> &functions, Settings &settings)
{
	std::function<void(const std::filesystem::path &path)> parse_directory;
	parse_directory = [&](const std::filesystem::path &path)
	{
		for (const auto &ele : std::filesystem::directory_iterator(path))
		{
			if (ele.is_regular_file())
			{
				if (std::any_of(ACCEPTED_EXTENSIONS.begin(), ACCEPTED_EXTENSIONS.end(), [&](const std::string &ex)
					{ return ele.path().extension() == ex; }))
				{
					std::cout << std::format("  - {}", ele.path().generic_string()) << std::endl;
					parse_file(ele.path(), functions, settings.search_term);
				}
			}
			else if (ele.is_directory())
			{
				parse_directory(ele.path());
			}
		}
	};

	std::cout << std::format("Scanning for functions in '{}' and exporting into '{}'\n", settings.source.generic_string(), settings.destination.generic_string());
	if (std::filesystem::is_regular_file(settings.source))
	{
		parse_file(settings.source, functions, settings.search_term);
	}
	else if (std::filesystem::is_directory(settings.source))
	{
		// It's a directory
		parse_directory(settings.source);
	}
	else
	{
		std::cerr << "[ERROR] Source is neither a file nor directory... What did you feed me?";
	}

	return true;
}

// Usage:
//   function_finder.exe <source : file/directory> <destination : file> <search_term : one word string> <init_function_name : one word string>
// Example
//   function_finder.exe cmd_client.cpp output.hpp CONSOLE_COMMAND init_console_commands
int main(int arg_count, const char **args)
{
	if (arg_count == 0)
	{
		std::cerr << "[ERROR] Invalid input. see '--help'\n";
	}

	if (arg_count == 2)
	{
		std::string_view arg_1 = args[1];
		if (arg_1 == "--help")
		{
			std::cout << "Function Finder (V" << VERSION << ")\n";
			std::cout << "The purpose of this tool is to allow easy pre-processing of source files to extract functions to use as commands for terminals, consoles, games, etc. ";
			std::cout << "It scans your file/directory for function prefixed by your search-term and outputs a header-file containing wrappers, function argument information, and more.\n";
			std::cout << "\n";
			std::cout << "Usage:\n";
			std::cout << "    'function_finder.exe --help'\n";
			std::cout << "        Help message on how to use Function Finder\n";
			std::cout << "\n";
			std::cout << "    'function_finder.exe <input_path> <output_path> <search_term> <init_function_name>'\n";
			std::cout << "        Run the tool.\n";
			std::cout << "\n";
			std::cout << "        input_path:\n";
			std::cout << "            The file or folder you want to scan for functions. If scanning directories run 'function_finder --extensions' to \n";
			std::cout << "            see which file endings are included.\n";
			std::cout << "        output_path:\n";
			std::cout << "            The file you want to export to. This is presumed to be a header-file, so give it an extension accordingly (.hpp or .h).\n";
			std::cout << "        search_term:\n";
			std::cout << "            The term to search for. Should be a single word string using underscores. The idea here is to have a '#define' in the \n";
			std::cout << "            input code that you put before the functions you want to find. See 'function_finder.exe --example' for an example of client code.\n";
			std::cout << "        init_function_name:\n";
			std::cout << "            What to name the Function Finder initialization function. If the idea is to use these functions as console-commands\n";
			std::cout << "            you can name it something like 'init_console_commands'.\n";
			std::cout << "\n";
			std::cout << "    'function_finder.exe --extensions'\n";
			std::cout << "        Get a list of file-extensions that are included when scanning directories.\n";
			std::cout << "\n";
			std::cout << "    'function_finder.exe --example'\n";
			std::cout << "        Get a full example use case of this tool\n";
		}
		if (arg_1 == "--extensions")
		{
			std::cout << "When scanning directories, the following extensions are included in the search. There is currently no way of adding to this.\n";
			std::cout << "  - ";
			for (auto ext = ACCEPTED_EXTENSIONS.begin(); ext != ACCEPTED_EXTENSIONS.end(); ++ext)
			{
				std::cout << "'" << *ext << "'";
				if(ext != std::prev(ACCEPTED_EXTENSIONS.end()))
				{
					std::cout << ", ";
				}
			}
			std::cout << "\n";
		}
		if (arg_1 == "--example")
		{
			std::cout << "// Example of usage code:\n";
			std::cout << "// For this example, run Function Finder with these arguments 'client.cpp output.hpp MY_COMMAND init_my_commands':\n";
			std::cout << "// client.cpp\n";
			std::cout << "\n";
			std::cout << "#include <iostream>\n";
			std::cout << "#include <format>\n";
			std::cout << "#include \"function_finder/function_finder.hpp\"\n";
			std::cout << "#include \"output.hpp\"\n";
			std::cout << "\n";
			std::cout << "#define MY_COMMAND\n";
			std::cout << "\n";
			std::cout << "int main()\n";
			std::cout << "{\n";
			std::cout << "\n";
			std::cout << "    Function_Map commands;\n";
			std::cout << "    init_my_commands(commands);\n";
			std::cout << "\n";
			std::cout << "    // This is the variable we output all the commands to:\n";
			std::cout << "    Value value;\n";
			std::cout << "\n";
			std::cout << "    std::cout << \"Correct usage:\\n\";\n";
			std::cout << "    std::vector<std::string> args = {\"5\", \"1.2f\", \"42.0\", \"Not using the default!\", \"false\"};\n";
			std::cout << "    std::cout << \"    \";\n";
			std::cout << "    commands[\"some_command\"].function(args, value);\n";
			std::cout << "    std::cout << \"    \" << value_to_string(value) << \"\\n\";\n";
			std::cout << "    std::cout << \"\\n\";\n";
			std::cout << "\n";
			std::cout << "    std::cout << \"Correct usage using defaults:\\n\";\n";
			std::cout << "    args = {\"5\", \"1.2f\", \"42.0\"};\n";
			std::cout << "    std::cout << \"    \";\n";
			std::cout << "    commands[\"some_command\"].function(args, value);\n";
			std::cout << "    std::cout << \"    \" << value_to_string(value) << \"\\n\";\n";
			std::cout << "    std::cout << \"\\n\";\n";
			std::cout << "\n";
			std::cout << "    std::cout << \"Correct usage, wrong type:\\n\";\n";
			std::cout << "    args = {\"5\", \"Woups! A string doesn't go here!\", \"5\"};\n";
			std::cout << "    std::cout << \"    \";\n";
			std::cout << "    commands[\"some_command\"].function(args, value);\n";
			std::cout << "    std::cout << \"\\n\";\n";
			std::cout << "\n";
			std::cout << "    std::cout << \"Correct usage, wrong number of arguments:\\n\";\n";
			std::cout << "    args = {\"5\"};\n";
			std::cout << "    std::cout << \"    \";\n";
			std::cout << "    commands[\"some_command\"].function(args, value);\n";
			std::cout << "    std::cout << \"\\n\";\n";
			std::cout << "\n";
			std::cout << "    // To make it easier to integrate with auto-complete and whatever else you'd want, the Function_Map includes much more information about the functions:\n";
			std::cout << "    std::cout << \"To make it easier to integrate with auto-complete and whatever else you'd want, the Function_Map includes much more information about the functions:\\n\";\n";
			std::cout << "    Function_Decl &command = commands[\"some_command\"];\n";
			std::cout << "    std::cout << \" - File:\" << command.file << \"\\n\";\n";
			std::cout << "    std::cout << \" - Line:\" << command.line << \"\\n\";\n";
			std::cout << "    std::cout << \" - Name:\" << command.name << \"\\n\";\n";
			std::cout << "    std::cout << \" - Note (The comment after the search term):\" << command.note << \"\\n\";\n";
			std::cout << "    std::cout << \" - Return type:\" << type_to_string(command.return_type) << \"\\n\";\n";
			std::cout << "    std::cout << \" - Argument information (types, names, and default arguments.):\";\n";
			std::cout << "    for(const auto &arg : command.arguments)\n";
			std::cout << "    {\n";
			std::cout << "        if(arg.has_default_value)\n";
			std::cout << "        {\n";
			std::cout << "            std::cout << std::format(\"    - {} {} = {}\\n\", type_to_cpp_type(arg.type), arg.name, value_to_string(arg.default_value));\n";
			std::cout << "        }\n";
			std::cout << "        else\n";
			std::cout << "        {\n";
			std::cout << "            std::cout << std::format(\"    - {} {}\\n\", type_to_cpp_type(arg.type), arg.name);\n";
			std::cout << "        }\n";
			std::cout << "    }\n";
			std::cout << "\n";
			std::cout << "    return 0;\n";
			std::cout << "}\n";
			std::cout << "\n";
			std::cout << "// This is the command we want it to find!\n";
			std::cout << "MY_COMMAND // I can even give it a comment to act as documentation!\n";
			std::cout << "bool some_command(int i, float f, double d, std::string s = \"default_values_are_supported\", bool b = false)\n";
			std::cout << "{\n";
			std::cout << "    std::cout << std::format(\"Hello from 'some_command'! These were the arguments: i = {}, f = {}, d = {}, s = {}, b = {}\\n\", i, f, d, s, b);\n";
			std::cout << "    std::cout << \"Returning if i == 5!\\n\";\n";
			std::cout << "    return i == 5;\n";
			std::cout << "}\n";
			
			std::cout << "\n";
		}
		return 0;
	}

	if (arg_count != 5)
	{
		std::cerr << "[ERROR] Needs a input path, output path, search_term, and init_function name as arguments. Terminating.\n";
		return 1;
	}

	Settings settings;
	settings.source = args[1];
	settings.destination = args[2];
	settings.search_term = args[3];
	settings.init_function_name = args[4];

	// Import function declarations
	std::vector<Function_Decl> functions;

	import_functions(settings.source, functions, settings);
	export_functions(settings, functions);

}