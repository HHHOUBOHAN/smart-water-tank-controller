# =============================================================================
# User-owned source code
# =============================================================================

# Automatically collect all user C source files under User/Src.
file(GLOB_RECURSE USER_C_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_LIST_DIR}/Src/*.c"
)

# Add collected source files to the main firmware target.
if(USER_C_SOURCES)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${USER_C_SOURCES}
    )
endif()

# Make User/Inc the header search root.
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/Inc"
)

# Temporary diagnostics. These three lines can be removed later.
message(STATUS "User source root: ${CMAKE_CURRENT_LIST_DIR}/Src")
message(STATUS "User include root: ${CMAKE_CURRENT_LIST_DIR}/Inc")
message(STATUS "User C sources: ${USER_C_SOURCES}")