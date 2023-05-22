cmake_minimum_required(VERSION 3.26)

add_executable(Help_Example null.cpp)

target_link_libraries(Help_Example PRIVATE Function_Finder_Lib)

set_property(TARGET Help_Example PROPERTY CXX_STANDARD 20)


set(input_dir "${CMAKE_CURRENT_SOURCE_DIR}")
set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/include")
set(output_file "${output_dir}/output.hpp")

if(FUNCTION-FINDER_BUILD_FROM_SOURCE)
    add_dependencies(Help_Example Function_Finder_Exe)
endif(FUNCTION-FINDER_BUILD_FROM_SOURCE)


add_custom_command(TARGET Help_Example
    PRE_BUILD
    COMMAND ${FUNCTION-FINDER_EXE_PATH} --example > help_example.cpp
    COMMAND ${FUNCTION-FINDER_EXE_PATH} "${input_dir}" "${output_file}" "MY_COMMAND" init_my_commands _new_wrapper_
)

target_sources(Help_Example PRIVATE help_example.cpp)

#
target_include_directories(Help_Example PRIVATE ${output_dir})