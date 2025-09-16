cmake_minimum_required(VERSION 3.16)
project(CerebrasRestClient)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_SYSROOT "/Library/Developer/CommandLineTools/SDKs/MacOSX14.5.sdk")

# Set policy to suppress Boost deprecation warning
cmake_policy(SET CMP0167 NEW)

# Find required packages
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

# Find OpenSSL (required for SSL support)
find_package(OpenSSL REQUIRED)

# Add executable
add_executable(rest_client rest_client.cpp)

# Include directories
target_include_directories(rest_client PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(rest_client PRIVATE
    Boost::headers  # For header-only libraries like Beast, SSL, and Asio
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(rest_client PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directory
set_target_properties(rest_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
