cmake_minimum_required(VERSION 3.26)

# This is the library bit. This is the common interface provided by the Function_Finder tool.
# Should be linked to the client.
add_library(Function_Finder_Lib INTERFACE)
target_include_directories(Function_Finder_Lib INTERFACE include)
set_property(TARGET Function_Finder_Lib PROPERTY CXX_STANDARD 20)


# This is the executable bit. This is what is running the actual preprocessor. This is not linked
# to by the client.
option(FUNCTION-FINDER_BUILD_FROM_SOURCE "Build Function Finder from Source. Use if you want to compile it from scratch, rather than using a precompiled exe." true)

if(FUNCTION-FINDER_BUILD_FROM_SOURCE)
    add_executable(Function_Finder_Exe function_finder.cpp function_finder_internal.hpp include/function_finder/function_finder.hpp)
    target_link_libraries(Function_Finder_Exe PRIVATE Function_Finder_Lib)
    set_property(TARGET Function_Finder_Exe PROPERTY CXX_STANDARD 20)
    set_target_properties(Function_Finder_Exe PROPERTIES OUTPUT_NAME "function_finder")

    set_target_properties(Function_Finder_Exe PROPERTIES
                            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin$<$<CONFIG:Debug>:>)    
    set(FUNCTION-FINDER_EXE_PATH "${PROJECT_SOURCE_DIR}/bin/function_finder.exe" CACHE STRING "Path to the Function Finder executable. If compiling from source this is automatically set." FORCE)
else()
    set(FUNCTION-FINDER_EXE_PATH "" CACHE STRING "Path to the Function Finder executable. If compiling from source this is automatically set.")
endif(FUNCTION-FINDER_BUILD_FROM_SOURCE)


