/**
DOCUMENTATION GOES HERE. What should even be in this header comment?
\todo Add support for namespaced-functions
*/
#pragma once

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

/// <summary>
/// Current Function Finder version
/// </summary>
const int VERSION = 1;

/// <summary>
/// The file-extensions that will be picked up when importing directories
/// </summary>
const std::vector<std::string> ACCEPTED_EXTENSIONS = {
	".cpp", ".hpp", ".h", ".c", ".cxx" };

/// <summary>
/// A simple struct for passing around command line settings fed to the program.
/// </summary>
struct Settings
{
	/// <summary>
	/// The path to the source file or directory.
	/// </summary>
	std::filesystem::path source;

	/// <summary>
	/// The path to generated destination file.
	/// </summary>
	std::filesystem::path destination;

	/// <summary>
	/// The term to search for.
	/// </summary>
	std::string search_term;

	/// <summary>
	/// The name of the initialization function.
	/// </summary>
	std::string init_function_name;

	/// <summary>
	/// The prefix to add to wrapper functions. So if you have a registered function "add" and a prefix
	/// "_function_" then the wrapper function will be called "_function_add"
	/// </summary>
	std::string wrapper_function_prefix;
};

/// <summary>
/// A wrapper for std::ostream specializing it for writing code files. This includes handling of
/// indentation and automatic endlines
/// </summary>
class Cpp_File_Writer
{
private:
	/// <summary>
	/// The number of spaces to use per indentation level.
	/// </summary>
	const int tab_size = 4;

	/// <summary>
	/// Current indentation level.
	/// </summary>
	int current_indent = 0;

	/// <summary>
	/// The wrapped stream used for output
	/// </summary>
	std::ostream &stream;

	/// <summary>
	/// Whether we're appending to the end of a line. This means to NOT add indentation or to end
	/// the current line.	
	/// </summary>
	bool line_continuation_mode = false;

public:
	explicit Cpp_File_Writer(std::ostream &stream)
		:stream(stream)
	{
	}

	/// <summary>
	/// Override stream operator to handle indentation level before outputting the actual obj.
	/// Includes
	/// </summary>
	/// <typeparam name="T">Type of parameter to output</typeparam>
	/// <param name="obj">Object to output</param>
	/// <returns></returns>
	template <typename T>
	Cpp_File_Writer &operator<<(T const &obj)
	{
		// Handle indentation
		if (!line_continuation_mode)
		{
			for (int i = 0; i < current_indent * tab_size; i++)
			{
				stream << ' ';
			}
		}

		stream << obj;

		if (!line_continuation_mode)
		{
			stream << "\n";
		}

		// Return stream to allow chaining.
		return *this;
	}

	void enable_line_continuation_mode()
	{
		line_continuation_mode = true;
	}

	void disable_line_continuation_mode()
	{
		line_continuation_mode = false;
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
};

/// <summary>
/// Imports all the functions found in the source file/directory matching the search term. The 
/// imported functions are stored in the inout_functions parameter.
/// </summary>
/// <param name="settings">The settings to use when importing, most relevant is the source path.
/// </param>
/// <param name="inout_functions">The list of all parsed functions.</param>
/// <returns>True if the import was successful and without issues.</returns>
bool import_functions(const Settings &settings, std::vector<Function_Decl> &inout_functions);

/// <summary>
/// Exports all the functions in the functions parameter to the destination file.
/// </summary>
/// <param name="settings">Settings to use when exporting.</param>
/// <param name="functions">The parsed functions.</param>
/// <returns>True if the export was successful.</returns>
bool export_functions(const Settings &settings, const std::vector<Function_Decl> &functions);

/// <summary>
/// Imports all functions found for the search term in files in a directory recursively.
/// </summary>
/// <param name="path">Path to the directory.</param>
/// <param name="functions">List of functions to import into.</param>
/// <param name="settings">Settings used for importing.</param>
/// <returns>True if import was successful.</returns>
bool import_directory(const std::filesystem::path &path, std::vector<Function_Decl> &functions,
	const Settings &settings);

/// <summary>
/// Imports all functions found for the search term in the file.
/// </summary>
/// <param name="path">Path to the directory.</param>
/// <param name="functions">List of functions to import into.</param>
/// <param name="settings">Settings used for importing.</param>
/// <returns>True if import was successful.</returns>
bool import_file(const std::filesystem::path &path, std::vector<Function_Decl> &inout_functions,
	const std::string_view search_term);

/// <summary>
/// Imports a function from source.
/// </summary>
/// <param name="source">A string_view starting with a function declaration to be parsed.</param>
/// <param name="out_function">Function to output into.</param>
/// <returns>True if successful.</returns>
bool import_function(std::string_view source, Function_Decl *out_function);



/**************************************
 *           Importer helpers         *
 **************************************/

 /// <summary>
 /// Fetches arguments for the arguments string pointed to by source.
 /// </summary>
 /// <param name="source">A string_view starting with a string parameter list.</param>
 /// <param name="out_args">List to store parsed arguments in.</param>
 /// <returns>True if successful.
 /// </returns>
bool get_arguments(std::string_view source, std::vector<Argument> &out_args);

/// <summary>
/// Parses an argument pointed to by source.
/// </summary>
/// <param name="source">A string_view starting with a string parameter</param>
/// <param name="out_arg">Where to store the parsed argument.</param>
/// <returns>The string length of the parsed argument. Return value can be used to step forward 
/// in source</returns>
size_t get_argument(std::string_view source, Argument &out_arg);

/// <summary>
/// Parses a C++ type into a \ref Value_Type pointed to by source.
/// </summary>
/// <param name="source">A string_view starting with a C++ type.</param>
/// <param name="out_type">Out parameter to store the parsed type in.</param>
/// <returns>The string length of the parsed type. Return value can be used to step forward 
/// in source</returns>
size_t get_type(std::string_view source, Value_Type &out_type);


/**************************************
 *           Exporter helpers         *
 **************************************/
void export_header(Cpp_File_Writer &w, const Settings &settings);
void export_pre_declarations(Cpp_File_Writer &w, const std::vector<Function_Decl> &functions);
void export_wrapper_functions(Cpp_File_Writer &w, const std::vector<Function_Decl> &functions, const Settings &settings);
void export_wrapper_function(Cpp_File_Writer &w, const Function_Decl &f, const Settings &settings);
void export_argument_handler(Cpp_File_Writer &w, size_t i, const Argument &arg);
void export_consumer_function_value_handler(Cpp_File_Writer &w, const Function_Decl &f);
void export_initialization_function(Cpp_File_Writer &w,
	const std::vector<Function_Decl> &functions, const Settings &settings);

std::string function_call_string(const Function_Decl &func);

/**************************************
 *           Printing functions       *
 **************************************/
void print_help();
void print_extensions();
void print_example();


/**************************************
 *             Utilities              *
 **************************************/
std::string read_file_to_string(const std::filesystem::path &path);
bool file_matches_extension(const std::filesystem::path &path);
