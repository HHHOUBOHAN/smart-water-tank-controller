cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary.
#

# =============================================================================
# Compiler settings
# =============================================================================

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# =============================================================================
# Build type
# =============================================================================

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# =============================================================================
# Project settings
# =============================================================================

set(CMAKE_PROJECT_NAME smart_water_tank_f103)

# Generate compile_commands.json for STM32Cube clangd.
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Enable C and assembly language support.
enable_language(C ASM)

# =============================================================================
# Main executable target
# =============================================================================

add_executable(${CMAKE_PROJECT_NAME})

# =============================================================================
# STM32CubeMX-generated code
# =============================================================================

add_subdirectory(cmake/stm32cubemx)

# =============================================================================
# User-owned code
# =============================================================================

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/User/CMakeLists.txt")
    message(FATAL_ERROR
        "Missing User/CMakeLists.txt. "
        "The User source-code module has not been initialized."
    )
endif()

add_subdirectory(User)

# =============================================================================
# Optional library search directories
# =============================================================================

target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user-defined library search directories here.
)

# =============================================================================
# Optional project macros
# =============================================================================

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user-defined macros here.
)

# Remove wrong libob.a dependency when C++ files are used.
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# =============================================================================
# Linked libraries
# =============================================================================

# Keep the same target_link_libraries style as the original Cube-generated file.
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user-defined libraries here.
)