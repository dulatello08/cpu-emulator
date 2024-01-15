cmake_minimum_required(VERSION 3.27)

# Set the project name
project(NeoCore C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wpedantic -g -O0 -D_THREAD_SAFE")

# Include directories
include_directories(.)

# Find SDL2 library
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Source files
file(GLOB SOURCES "*.c")
file(GLOB SOCKET_SOURCES "socket/*.c")
file(GLOB KEYBOARD_SOURCES "keyboard/*.c")

# Add executable
add_executable(emulator ${SOURCES} ${SOCKET_SOURCES} ${KEYBOARD_SOURCES})

# Link against libraries
target_link_libraries(emulator ${SDL2_LIBRARIES})

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
        COMMAND ${CMAKE_COMMAND} -P clean-all.cmake
)