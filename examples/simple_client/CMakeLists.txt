cmake_minimum_required(VERSION 3.26)

add_executable(Simple_Client client.cpp)

target_link_libraries(Simple_Client PRIVATE Function_Finder_Lib)

set_property(TARGET Simple_Client PROPERTY CXX_STANDARD 20)


set(input_dir "${CMAKE_CURRENT_SOURCE_DIR}")
set(output_dir "${CMAKE_CURRENT_BINARY_DIR}/include")
set(output_file "${output_dir}/output.hpp")

message ("EXECUTABLE: ${FUNCTION-FINDER_EXE_PATH} ")
message ("INPUT: ${input_dir}")
message ("OUTPUT: ${output_file}")

if(FUNCTION-FINDER_BUILD_FROM_SOURCE)
    add_dependencies(Simple_Client Function_Finder_Exe)
endif(FUNCTION-FINDER_BUILD_FROM_SOURCE)

add_custom_command(TARGET Simple_Client
    PRE_BUILD
    COMMAND ${FUNCTION-FINDER_EXE_PATH} "${input_dir}" "${output_file}" "MY_COMMAND" init_my_commands
)

target_include_directories(Simple_Client PRIVATE ${output_dir})