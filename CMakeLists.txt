cmake_minimum_required(VERSION 3.25)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

# Set the project name
project(NeoCore C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wpedantic -g -O0 -D_THREAD_SAFE")

# Include directories
include_directories(.)


find_package(SDL2 REQUIRED)
if(NOT SDL2_FOUND)
    message(FATAL_ERROR "SDL2 not found!")
endif()

include_directories(${SDL2_INCLUDE_DIRS})

# Find SDL2_ttf library
find_package(SDL2_ttf REQUIRED)
if(NOT SDL2_ttf_FOUND)
    message(FATAL_ERROR "SDL2_ttf not found!")
endif()

include_directories(${SDL2_TTF_INCLUDE_DIRS})

# Source files
file(GLOB SOURCES "*.c")
file(GLOB SOCKET_SOURCES "socket/*.c")
file(GLOB KEYBOARD_SOURCES "keyboard/*.c")
file(GLOB GUI_SOURCES "gui/*.c")
file(GLOB IPC_SOURCES "ipc/*.c*")

# Remove socket sources from main emulator sources
#list(REMOVE_ITEM SOURCES ${SOCKET_SOURCES})

# Add executable for emulator (without socket sources)
add_executable(emulator ${SOURCES} ${IPC_SOURCES})
# Link against libraries
target_link_libraries(emulator)

# Add executable for emulator_socket (with socket sources)
add_executable(emulator_socket ${SOURCES} ${SOCKET_SOURCES} ${IPC_SOURCES})
# Define EMULATOR_SOCKET
target_compile_definitions(emulator_socket PRIVATE EMULATOR_SOCKET)
# Link against libraries
target_link_libraries(emulator_socket)

# Add executable for gui_subsystem
add_executable(gui_subsystem ${GUI_SOURCES} ${IPC_SOURCES})

# Include directories for gui_subsystem
target_include_directories(gui_subsystem PRIVATE gui)

# Link against SDL2 for gui_subsystem
target_link_libraries(gui_subsystem ${SDL2_LIBRARIES} SDL2::TTF)

# Platform-specific settings
if(UNIX AND NOT APPLE)
    target_compile_definitions(emulator PRIVATE LINUX)
    target_compile_definitions(emulator_socket PRIVATE LINUX)
    # Additional include and library directories for Linux
elseif(APPLE)
    target_compile_definitions(emulator PRIVATE DARWIN)
    target_compile_definitions(emulator_socket PRIVATE DARWIN)
    # Additional include and library directories for Darwin (macOS)
endif()

# Custom target for cleaning extra files (optional)
add_custom_target(clean-all
        COMMAND
)
