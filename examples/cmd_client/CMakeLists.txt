cmake_minimum_required(VERSION 3.26)

add_executable(Cmd_Client cmd_client.cpp cmd_client.hpp)

target_link_libraries(Cmd_Client PRIVATE Function_Finder_Lib)

set_property(TARGET Cmd_Client PROPERTY CXX_STANDARD 20)


set(input_dir "${CMAKE_CURRENT_SOURCE_DIR}")
set(output_dir "${CMAKE_BINARY_DIR}/generated/include")
set(output_file "${output_dir}/output.hpp")

message ("EXECUTABLE: ${FUNCTION-FINDER_EXE_PATH} ")
message ("INPUT: ${input_dir}")
message ("OUTPUT: ${output_file}")


add_custom_command(TARGET Cmd_Client
    PRE_BUILD
    COMMAND ${FUNCTION-FINDER_EXE_PATH} "${input_dir}" "${output_file}"
)

target_include_directories(Cmd_Client PRIVATE ${output_dir})