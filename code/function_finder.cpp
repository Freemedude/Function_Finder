/*
See "include/function_finder/function_finder.hpp" for usage guide.
Copyright Daniel 2023
*/

#include "function_finder_internal.hpp"
#include <cassert>

int main(int arg_count, const char **args)
{
	// Parse arguments
	if (arg_count == 0)
	{
		std::cerr << "[ERROR] Invalid input. see '--help'\n";
	}

	if (arg_count == 2)
	{
		std::string_view arg_1 = args[1];
		if (arg_1 == "--help")
		{
			print_help();
		}
		if (arg_1 == "--extensions")
		{
			print_extensions();
		}
		if (arg_1 == "--example")
		{
			print_example();
		}
		return 0;
	}

	if (arg_count != 6)
	{
		std::cerr << "[ERROR] Needs a input path, output path, search_term, "
			"init_function name, and wrapper function prefix as arguments. Terminating.\n";
		return 1;
	}

	// Save settings
	Settings settings;
	settings.source = args[1];
	settings.destination = args[2];
	settings.search_term = args[3];
	settings.init_function_name = args[4];
	settings.wrapper_function_prefix = args[5];

	std::vector<Function_Decl> functions;
	import_functions(settings, functions);

	export_functions(settings, functions);
}

bool import_functions(const Settings &settings, std::vector<Function_Decl> &inout_functions)
{
	// Print a helpful message to stdout
	std::cout << std::format("Scanning for functions in '{}' and exporting into '{}'\n",
		settings.source.generic_string(), settings.destination.generic_string());

	if (std::filesystem::is_regular_file(settings.source))
	{
		return import_file(settings.source, inout_functions, settings);
	}
	else if (std::filesystem::is_directory(settings.source))
	{
		// It's a directory
		return import_directory(settings.source, inout_functions, settings);
	}

	std::cerr << "[ERROR] Source is neither a file nor directory... What did you feed me?\n";
	return false;
}


bool import_directory(const std::filesystem::path &path,
	std::vector<Function_Decl> &inout_functions,
	const Settings &settings)
{
	for (const auto &ele : std::filesystem::directory_iterator(path))
	{
		// Check whether it's a matching file, or a directory.
		if (ele.is_regular_file() && file_matches_extension(ele.path()))
		{
			std::cout << std::format("  - {}\n", ele.path().generic_string());
			if (!import_file(ele.path(), inout_functions, settings))
			{
				return false;
			}
		}
		else if (ele.is_directory())
		{
			if (!import_directory(ele.path(), inout_functions, settings))
			{
				return false;
			}
		}
	}
	return true;
}

bool matches_search_term(std::string_view source, const Settings &settings)
{
	if(source.size() < settings.search_term.size() + 1)
	{
		return false;
	}
	char character_after_term = source[settings.search_term.size()];
	
	bool matches = source.starts_with(settings.search_term) && 
		(std::isspace(character_after_term) || character_after_term == '/'); 
	return matches;	
}


bool import_file(const std::filesystem::path &path, std::vector<Function_Decl> &inout_functions,
	const Settings &settings)
{
	std::string content_str = read_file_to_string(path);
	std::string_view content_view = content_str;

	// Loop over every line in the file, checking to see if it starts with the search term.
	for (size_t line = 1; true; line++)
	{
		if (matches_search_term(content_view, settings))
		{
			Function_Decl func;
			func.line = line + 1;
			func.file = path.generic_string();

			auto func_view = skip_whitespace(content_view);

			bool success = import_function(func_view, &func, settings);
			if (!success)
			{
				return false;
			}

			inout_functions.push_back(func);
		}

		// Check for end of file.
		auto next_line_end = content_view.find('\n');
		if (next_line_end == std::string_view::npos)
		{
			break;
		}
		content_view = content_view.substr(next_line_end + 1);
	}

	return true;
}

bool import_function(std::string_view source, Function_Decl *out_function, const Settings &setting)
{
	// Skip the search term
	auto current = advance(source, setting.search_term.size());
	current = skip_whitespace(current);

	// Check for note
	std::string note;
	current = skip_whitespace(current);
	auto comment_size = get_comment(current, note);
	if(comment_size)
	{
		out_function->note = note;
		current = advance(current, comment_size);
	}


	// Skip any other tags by waiting until we get either "inline" or some return type.
	bool found_function = false;
	while(true)
	{
		current = skip_whitespace(current);
		
		// Break if it says "inline"...
		if(current.starts_with("inline"))
		{
			break;
		}

		// ... or if it's a supported type we support.
		Value_Type type;
		size_t type_length = get_type(current, type);
		if(type_length && type != Value_Type::UNKNOWN)
		{
			break;
		}

		// If we get here there's another tag term or something else we don't support!
		// Using get_symbol is not exactly right, since this would also support colons which is wrong.
		std::string tag;
		auto tag_size = get_symbol(current, tag);
		if(tag_size)
		{
			current = advance(current, tag_size);
			std::string other_tag_comment;
			auto comment_size = get_comment(current, other_tag_comment);
			if(comment_size)
			{
				current = advance(current, comment_size);
			}
		}
	}


	// Check for 'inline' modifier
	current = skip_whitespace(current);
	if (current.starts_with("inline"))
	{
		current = advance(current, sizeof("inline"));
	}

	size_t final_length = get_type(current, out_function->return_type);
	current = advance(current, final_length);
	current = skip_whitespace(current);

	size_t length = get_symbol(current, out_function->name);
	if (!length)
	{
		std::cerr << "[ERROR] Failed to get function name '" << current << "'\n";
		return false;
	}

	final_length += length;
	current = advance(current, length);

	bool success = get_arguments(current, out_function->arguments);
	if (!success)
	{
		std::cerr << "[ERROR] Failed to get arguments for function '" << out_function->name << "'\n";
		return false;
	}

	out_function->num_optional_args = (int)std::count_if(out_function->arguments.begin(), out_function->arguments.end(), [](Argument &arg)
		{ return arg.has_default_value; });
	out_function->num_required_args = out_function->arguments.size() - out_function->num_optional_args;

	return true;
}


bool export_functions(const Settings &settings, const std::vector<Function_Decl> &functions)
{
	// Create file and needed folders to get there.
	std::filesystem::create_directories(settings.destination.parent_path());
	std::ofstream file(settings.destination);
	if (!file.is_open())
	{
		std::cerr << std::format("[ERROR] Could not create destination file '{}'. Terminating\n",
			settings.destination.generic_string());
		return false;
	}

	Cpp_File_Writer w(file);

	export_header(w, settings);
	export_pre_declarations(w, functions);
	export_wrapper_functions(w, functions, settings);
	export_initialization_function(w, functions, settings);

	file.close();
	return true;
}

// Returns the size of the comment. If it returns 0 then there is no comment.
// Does not skip whitespace, do it yourself!
size_t get_comment(std::string_view source, std::string &out_result)
{
	auto current = source;

	// Confirm that it's a comment
	if(current.size() < 2 || current[0] != '/' || (current[1] != '/' && current[1] != '*'))
	{
		return 0;
	}

	bool is_line_comment = current[1] == '/';

	current = advance(current, (size_t) 2);
	current = skip_whitespace(current);

	if (is_line_comment)
	{
		auto end = current.find('\n');
		if(end == std::string_view::npos)
		{
			// This is the end of a file without a newline, so find string end instead.
			end = current.find('\0');
			assert(end != std::string_view::npos);
			if(end != std::string_view::npos)
			{
				return 0;
			}
			
		}

		auto comment_end = end;

		// Skip whitespaces BACKWARDS
		while(std::isspace(current[comment_end - 1]) && comment_end > 0)
		{
			comment_end--;
		}
		
		
		out_result = std::string(current.begin(), current.begin() + comment_end);
		current = advance(current, end + 1);
	}
	else
	{		
		// Find the end of the comment.
		auto end = current.find("*/");
		assert(end != std::string_view::npos);

		if(end == std::string_view::npos)
		{
			std::cout << "Could not find */ in \n" << current << '\n';
			return 0;
		}

		auto comment_end = end;

		// Skip whitespaces BACKWARDS
		while(std::isspace(current[comment_end - 1]) && comment_end > 0)
		{
			comment_end--;
		}
		
		out_result = std::string(current.begin(), current.begin() + comment_end);

		// Sanitize.
		out_result = std::regex_replace(out_result, std::regex("\n"), "\\n");
		current = advance(current, end + 2);
	}

	auto result = source.size() - current.size();
	return result;

}

size_t get_type(std::string_view source, Value_Type &out_type)
{
	size_t result = 0;
	
	auto current = skip_whitespace(source);

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

size_t parse_type(std::string_view source, Value_Type type, Value &out_result)
{
	size_t length = 0;

	switch (type)
	{
	case Value_Type::BOOLEAN:
		length = get_bool(source, out_result.data.bool_value);
		break;
	case Value_Type::FLOAT:
		length = get_float(source, out_result.data.float_value);
		break;
	case Value_Type::DOUBLE:
		length = get_double(source, out_result.data.double_value);
		break;
	case Value_Type::INTEGER:
		length = get_int(source, out_result.data.int_value);
		break;
	case Value_Type::STRING:
		{
		std::string result;
		length = get_quoted_string(source, result);
		if (length)
		{
			strncpy(out_result.data.string_value, result.c_str(), result.length());
		}
		break;
		}
	}

	return length;
}

size_t get_argument(std::string_view source, Argument &out_arg)
{
	auto current = skip_whitespace(source);

	// Get the type
	Value_Type type;
	auto type_length = get_type(current, type);
	current = advance(current, type_length);

	// Get the argument name
	current = skip_whitespace(current);
	std::string name;
	size_t name_length = get_symbol(current, name);
	if (!name_length)
	{
		std::cout << std::format("[ERROR] Failed to get argument name at position '{}'\n", current);
		return 0;
	}
	out_arg.name = name;
	out_arg.type = type;

	current = advance(current, name_length);
	current = skip_whitespace(current);

	// Check for default value
	if (current.size() > 0 && current[0] == '=')
	{
		out_arg.has_default_value = true;
		out_arg.default_value.type = type;

		current = advance(current, (size_t)1);
		current = skip_whitespace(current);
		
		auto default_arg_length = parse_type(current, type, out_arg.default_value);
		if(default_arg_length)
		{
			current = advance(current, default_arg_length);
		}
	}

	current = skip_whitespace(current);
	//std::cout << name << " after skipping whitespace: " << current << '(' << source.size() - current.size() << " of " << source.size() << ')' <<   std::endl;
	
	// Check for note.
	std::string note;
	size_t comment_length = get_comment(current, note);
	//std::cout << "after getting comment: " << current << '(' << source.size() - current.size() << " of " << source.size() << ')' <<   std::endl;
	if(comment_length)
	{
		//std::cout << "There is a comment: '" << note << "'\n";
		current = advance(current, comment_length);
		out_arg.note = note;
	}

	//std::cout << "after comment_length check: " << current << '(' << source.size() - current.size() << " of " << source.size() << ')' <<   std::endl;
	
	return source.size();
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
		bool in_line_comment = false;
		bool in_block_comment = false;
		size_t length = 0;

		// Find the end of the argument
		while (true)
		{
			// Start line comment
			if (source[length] == '/' && source[length + 1] == '/')
			{
				in_line_comment = true;;
			}
			
			// End line comment
			if(in_line_comment && source[length] == '\n')
			{
				in_line_comment = false;
			}

			// Start block comment
			if (source[length] == '/' && (source[length + 1] == '*'))
			{
				in_block_comment = true;
			}

			// End block comment
			if (source[length] == '*' && (source[length + 1] == '/'))
			{
				in_block_comment = false;
			}

			bool in_comment = in_block_comment || in_line_comment;

			// We've reached the end of the argument if we've reached a comma not in a comment.
			if (!in_comment && source[length] == ',')
			{
				length++;
				break;
			}

			// We've reached the end of all arguments if we're found an end parenthesis that's not 
			// in a comment.
			if (!in_comment && source[length] == ')')
			{
				if(length > 0)
				{
					length++;
				}
				found_end_parenthesis = true;
				break;
			}

			length++;
		}

		if (length > 0)
		{
			auto arg_source = source.substr(0, length - 1);

			Argument arg{};
			bool success = get_argument(arg_source, arg);

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


/**************************************
 *           Exporter helpers         *
 **************************************/

void export_header(Cpp_File_Writer &w, const Settings &settings)
{
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
}

void export_pre_declarations(Cpp_File_Writer &w, const std::vector<Function_Decl> &functions)
{
	// Write a section header to make it easier to navigate the output file.
	w << "//////////////////////////////";
	w << "//     PRE-DECLARATIONS     //";
	w << "//////////////////////////////";

	w << "// Pre-declarations of client-functions";
	w.skip_line();
	for (const auto &f : functions)
	{
		w << std::format("// From \"{}\" L{}", f.file, f.line);
		w.enable_line_continuation_mode();
		w << value_type_to_cpp_type(f.return_type) << " " << f.name << "(";
		for (int i = 0; i < f.arguments.size(); i++)
		{
			const Argument &arg = f.arguments[i];
			w << value_type_to_cpp_type(arg.type) << " " << arg.name;

			// NOTE: We intentionally leave out the default value here. Since the compiler will 
			// complain if we define it twice. Default arguments are handled in the generated
			// wrapper functions placed below.
			if (i != f.arguments.size() - 1)
			{
				w << ", ";
			}
		}
		w << ");\n";
		w.disable_line_continuation_mode();
		w.skip_line();
	}
}

void export_wrapper_functions(Cpp_File_Writer &w, const std::vector<Function_Decl> &functions, 
	const Settings &settings)
{
	// Write a section header to make it easier to navigate the output file.
	w << "//////////////////////////////";
	w << "//         WRAPPERS         //";
	w << "//////////////////////////////";
	w.skip_line();

	// Write the wrapper functions
	for (auto &function : functions)
	{
		export_wrapper_function(w, function, settings);
	}
}

void export_wrapper_function(Cpp_File_Writer &w, const Function_Decl &f, const Settings &settings)
{
	// Write the function definition
	w << std::format("// Generated based on function \"{}\" from file \"{}\" L{}",
		f.name, f.file, f.line);
	w << std::format("inline Call_Result {}{}(std::vector<std::string> &args, bool call_client_function)",
		settings.wrapper_function_prefix, f.name);
	w << "{";
	w.indent();
	w << "Call_Result call_result;";
	w.skip_line();

	// Write the check to confirm that all needed arguments are provided
	if (f.num_required_args > 0)
	{
		w << "// Check that all required arguments are provided.";
		w << std::format("if(args.size() < {})", f.num_required_args);
		w << "{";
		w.indent();
		w << std::format("call_result.error_message = std::format(\"Not enough arguments for '{}'. Needed {}, but got {{}}\", args.size());",
			f.name, f.num_required_args);
		w << "call_result.status = Call_Result_Status::NOT_ENOUGH_ARGUMENTS_ERROR;";
		w << "call_result.error_helper_value = args.size();";
		w << "return call_result;";

		w.unindent();
		w << "}";
		w.skip_line();
	}

	// Write the 'success' flag. This flag is reused to confirm all arguments are parsed correctly.
	if (f.arguments.size() > 0)
	{
		w << "// Reused success flag used for value parsing";
		w << "int success = false;";
		w.skip_line();
	}

	// Write the argument handler for each argument
	for (int i = 0; i < f.arguments.size(); i++)
	{
		export_argument_handler(w, i, f.arguments[i]);
	}

	export_consumer_function_value_handler(w, f);

	w.skip_line();

	w << "return call_result;";

	w.unindent();
	w << "}";
	w.skip_line();
}

void export_argument_handler(Cpp_File_Writer &w, size_t i, const Argument &arg)
{
	// Create variable
	w << std::format("// {} argument {}: '{} {}'", arg.has_default_value ? "Optional" : "Required",
		i, value_type_to_cpp_type(arg.type), arg.name);

	if (arg.has_default_value)
	{
		w << std::format("{} arg_{} = {};", value_type_to_cpp_type(arg.type), arg.name,
			to_string(arg.default_value));

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
			w << std::format("success = get_{}(args[{}], arg_{});",
				value_type_to_readable_string(arg.type), i, arg.name);
		}
	}
	else
	{
		w << std::format("{} arg_{};", value_type_to_cpp_type(arg.type), arg.name);
		if (arg.type == Value_Type::STRING)
		{
			w << "success = true;";
			w << std::format("arg_{} = args[{}];", arg.name, i);
		}
		else
		{
			w << std::format("success = get_{}(args[{}], arg_{});", 
				value_type_to_readable_string(arg.type), i, arg.name);
		}
	}

	w << "if(!success)";
	w << "{";
	w.indent();

	w << std::format("call_result.error_message = std::format(\"Failed to parse argument {0} '{1}'. Attempted to parse a {2}, but "
		"got string '{{}}'\", args[{0}]);", i, arg.name, value_type_to_cpp_type(arg.type));
	w << "call_result.status = Call_Result_Status::ARGUMENT_PARSING_ERROR;";
	w << std::format("call_result.error_helper_value = {};", i);
	w << "return call_result;";

	w.unindent();
	w << "}";

	if (arg.has_default_value)
	{
		w.unindent();
		w << "}";
	}
	w.skip_line();
}


void export_consumer_function_value_handler(Cpp_File_Writer &w, const Function_Decl &f)
{
	w << std::format("call_result.value.type = {};", to_string(f.return_type));
	w << "call_result.status = Call_Result_Status::SUCCESS;";
	w << "if(!call_client_function)";
	w.indent();
	w << "return call_result;";
	w.unindent();
	
	if (f.return_type == Value_Type::VOID)
	{
		w << std::format("{};", function_call_string(f));
	}
	else if (f.return_type == Value_Type::STRING)
	{
		w << std::format("std::string result = {};", function_call_string(f));
		w << "strncpy(call_result.value.data.string_value, result.data(), "
			"sizeof(call_result.value.data.string_value));";
	}
	else
	{
		w << std::format("call_result.value.data.{}_value = {};", 
			value_type_to_readable_string(f.return_type), function_call_string(f));
	}
}

void export_initialization_function(Cpp_File_Writer &w, 
	const std::vector<Function_Decl> &functions, const Settings &settings)
{
	// Write the initialization function
	w << "//////////////////////////////";
	w << "//       INITIALIZER        //";
	w << "//////////////////////////////";
	w.skip_line();

	w << std::format("void {}(Function_Map &out_functions)", settings.init_function_name);
	w << "{";
	w.indent();

	for (const auto &f : functions)
	{
		w << std::format("out_functions[\"{0}\"] = "
			"Function_Decl(\"{0}\", {4}{0}, {1}, {2}, {3},",
			f.name, to_string(f.return_type), f.num_required_args, f.num_optional_args, 
			settings.wrapper_function_prefix);
		w.indent();
		w << std::format("\"{}\", (size_t){},", f.file, f.line);
		w << "// Arguments";
		w << "{";
		w.indent();
		for (size_t i = 0; i < f.arguments.size(); i++)
		{
			const auto &arg = f.arguments[i];
			bool last_iter = i == f.arguments.size() - 1;

			if (arg.has_default_value)
			{
				w << std::format("Argument(\"{}\", {}, \"{}\", {}){}",
					arg.name, to_string(arg.type), arg.note, to_string(arg.default_value), 
					last_iter ? "" : ",");
			}
			else
			{
				w << std::format("Argument(\"{}\", {}, \"{}\"){}",
					arg.name, to_string(arg.type), arg.note, last_iter ? "" : ",");
			}
		}
		w.unindent();
		w << "},";
		w << std::format("\"{}\"", f.note);
		w.unindent();
		w << ");";
		w.skip_line();
	}

	w.unindent();
	w << "}";
	w.skip_line();
}

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


/**************************************
 *           Printing functions       *
 **************************************/

void print_help()
{
	std::cout << "Function Finder (V" << VERSION << ")\n";
	std::cout << R"(
The purpose of this tool is to allow easy pre-processing of source files to extract functions to use as commands for terminals, consoles, games, etc.
It scans your file/directory for function prefixed by your search-term and outputs a header-file containing wrappers, function argument information, and more.
See the "docs" folder for more details on exact function.

Usage:
    'function_finder.exe <input_path> <output_path> <search_term> <init_function_name> <wrapper_function_prefix>'
        Run the tool.

        input_path:
            The file or folder you want to scan for functions. If scanning directories run 'function_finder --extensions' to
            see which file endings are included.
        output_path:
            The file you want to export to. This is presumed to be a header-file, so give it an extension accordingly (.hpp or .h).
        search_term:
            The term to search for. Should be a single word string using underscores. The idea here is to have a '#define' in the
            input code that you put before the functions you want to find. See 'function_finder.exe --example' for an example of client code.
        init_function_name:
            What to name the Function Finder initialization function. If the idea is to use these functions as console-commands
            you can name it something like 'init_console_commands'.
        wrapper_function_prefix:
            What prefix to add to the auto-generated wrapper functions.

    'function_finder.exe --help'
        This help message on how to use Function Finder

    'function_finder.exe --extensions'
        Get a list of file-extensions that are included when scanning directories.

    'function_finder.exe --example'
        Get a full example use case of this tool

)";
}

void print_extensions()
{
	std::cout << "When scanning directories, the following extensions are included in the search. There is currently no way of adding to this.\n";
	std::cout << "  - ";
	for (auto ext = ACCEPTED_EXTENSIONS.begin(); ext != ACCEPTED_EXTENSIONS.end(); ++ext)
	{
		std::cout << "'" << *ext << "'";
		if (ext != std::prev(ACCEPTED_EXTENSIONS.end()))
		{
			std::cout << ", ";
		}
	}
	std::cout << '\n';
}

void print_example()
{
	std::cout << R"(
// Example of usage code:
// For this example, run Function Finder with these arguments 'client.cpp output.hpp MY_COMMAND init_my_commands my_wrapper':
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
    Call_Result result;

    std::cout << "Correct usage:\n";
    std::vector<std::string> args = {"5", "1.2f", "42.0", "Not using the default!", "false"};
    std::cout << "    ";
    result = commands["some_command"].function(args, true);
    std::cout << "    " << to_string(result.value) << '\n';
    std::cout << '\n';

    std::cout << "Correct usage using defaults:\n";
    args = {"5", "1.2f", "42.0"};
    std::cout << "    ";
    result = commands["some_command"].function(args, true);
    std::cout << "    " << to_string(result.value) << '\n';
    std::cout << '\n';

    std::cout << "Correct usage, wrong type:\n";
    args = {"5", "Woups! A string doesn't go here!", "5"};
    std::cout << "    ";
    result = commands["some_command"].function(args, true);
    std::cout << '\n';

    std::cout << "Correct usage, wrong number of arguments:\n";
    args = {"5"};
    std::cout << "    ";
    result = commands["some_command"].function(args, true);
    std::cout << '\n';

    // To make it easier to integrate with auto-complete and whatever else you'd want, the Function_Map includes much more information about the functions:\n";
    std::cout << "To make it easier to integrate with auto-complete and whatever else you'd want, the Function_Map includes much more information about the functions:\n";
    Function_Decl &command = commands["some_command"];
    std::cout << " - File: " << command.file << '\n';
    std::cout << " - Line: " << command.line << '\n';
    std::cout << " - Name: " << command.name << '\n';
    std::cout << " - Note (The comment after the search term): " << command.note << '\n';
    std::cout << " - Return type: " << to_string(command.return_type) << '\n';
    std::cout << " - Argument information (types, names, and default arguments.):";
    for(const auto &arg : command.arguments)
    {
        if(arg.has_default_value)
        {
            std::cout << std::format("    - {} {} = {}\n", value_type_to_cpp_type(arg.type), arg.name, to_string(arg.default_value));
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
)";
}

/**************************************
 *             Utilities              *
 **************************************/

std::string read_file_to_string(const std::filesystem::path &path)
{
	std::ifstream file(path);
	if (!file.is_open())
	{
		std::cerr << "[ERROR] Couldn't find file '" << path << "'! Terminating.\n";
		return "";
	}

	std::stringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

bool file_matches_extension(const std::filesystem::path &path)
{
	return std::find(ACCEPTED_EXTENSIONS.begin(),
		ACCEPTED_EXTENSIONS.end(), path.extension()) != ACCEPTED_EXTENSIONS.end();
}
