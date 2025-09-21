cmake_minimum_required(VERSION 3.16)
project(CerebrasRestClient)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add liboai submodule
add_subdirectory(extern/liboai)

# Find required packages
find_package(Boost REQUIRED COMPONENTS program_options)

# Add executables
add_executable(throughput_test throughput_test.cpp)

# Link libraries
target_link_libraries(throughput_test PRIVATE
    Boost::program_options  # For command line argument parsing
    oai  # liboai library
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(throughput_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directory
set_target_properties(throughput_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
