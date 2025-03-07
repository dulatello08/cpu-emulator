cmake_minimum_required(VERSION 3.25)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

# Set the project name
project(NeoCore LANGUAGES C CXX)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wpedantic -g -D_THREAD_SAFE")
# Check for x86 or x86_64 architectures
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|i[3-6]86)$")
    # Add AVX2 flag for x86 architectures
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2")
endif()
# Include directories
include_directories(.)


# Source files
file(GLOB SOURCES "*.c")


add_executable(emulator ${SOURCES}
        mov.c)
# Link against libraries
target_link_libraries(emulator)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    target_compile_definitions(emulator PRIVATE LINUX)
    # Additional include and library directories for Linux
elseif(APPLE)
    target_compile_definitions(emulator PRIVATE DARWIN)
    # Additional include and library directories for Darwin (macOS)
endif()

# Custom target for cleaning extra files (optional)
add_custom_target(clean-all
        COMMAND
)
