# =============================================================================
# User-owned source code
# =============================================================================
#
# Sources:
#   User/Src/**/*.c
#
# Headers:
#   User/Inc/**/*.h
#
# Include examples:
#   #include "BSP/board.h"
#   #include "BSP/buzzer.h"
#   #include "BSP/key.h"
#   #include "BSP/led.h"
#   #include "BSP/relay.h"
#

# Recursively collect all user C source files.
file(GLOB_RECURSE USER_C_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_LIST_DIR}/Src/*.c"
)

# Add user source files to the main STM32 executable target.
if(USER_C_SOURCES)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${USER_C_SOURCES}
    )
endif()

# Make User/Inc the header search root.
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/Inc"
)

# Temporary configure diagnostics.
message(STATUS "User source root: ${CMAKE_CURRENT_LIST_DIR}/Src")
message(STATUS "User include root: ${CMAKE_CURRENT_LIST_DIR}/Inc")
message(STATUS "User C sources: ${USER_C_SOURCES}")