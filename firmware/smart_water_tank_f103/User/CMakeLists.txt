# =============================================================================
# User/CMakeLists.txt
#
# 功能：
# 1. 自动收集 User/Src 下的所有 C 源文件
# 2. 将源文件加入 STM32 主固件目标
# 3. 将 User/Inc 设置为头文件搜索根目录
#
# 目录约定：
# User/
# ├── Inc/
# │   ├── BSP/
# │   ├── APP/
# │   ├── SERVICE/
# │   ├── NETWORK/
# │   ├── PROTOCOL/
# │   └── COMMON/
# └── Src/
#     ├── BSP/
#     ├── APP/
#     ├── SERVICE/
#     ├── NETWORK/
#     ├── PROTOCOL/
#     └── COMMON/
# =============================================================================


# -----------------------------------------------------------------------------
# 1. 检查STM32主固件目标是否已经创建
# -----------------------------------------------------------------------------

if(NOT TARGET ${CMAKE_PROJECT_NAME})
    message(FATAL_ERROR
        "Target '${CMAKE_PROJECT_NAME}' does not exist. "
        "Please call add_subdirectory(User) after add_executable() "
        "in the root CMakeLists.txt."
    )
endif()


# -----------------------------------------------------------------------------
# 2. 自动收集 User/Src 下的所有 .c 文件
# -----------------------------------------------------------------------------

file(GLOB_RECURSE USER_C_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    "${CMAKE_CURRENT_LIST_DIR}/Src/*.c"
)


# -----------------------------------------------------------------------------
# 3. 将用户源文件加入STM32主固件
# -----------------------------------------------------------------------------

if(USER_C_SOURCES)
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${USER_C_SOURCES}
    )
else()
    message(WARNING
        "No user C source files were found under: "
        "${CMAKE_CURRENT_LIST_DIR}/Src"
    )
endif()


# -----------------------------------------------------------------------------
# 4. 设置用户头文件搜索根目录
# -----------------------------------------------------------------------------
#
# 设置为 User/Inc 后，代码中统一这样引用：
#
# #include "BSP/key.h"
# #include "BSP/relay.h"
# #include "APP/app_control.h"
# #include "SERVICE/sensor_service.h"
#
# 不需要写：
#
# #include "User/Inc/BSP/key.h"
#
# -----------------------------------------------------------------------------

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/Inc"
)


# -----------------------------------------------------------------------------
# 5. 在IDE中按照真实目录显示源文件
# -----------------------------------------------------------------------------
#
# 这不会影响编译，只会让支持source_group的IDE显示更整齐。
#

if(USER_C_SOURCES)
    source_group(
        TREE "${CMAKE_CURRENT_LIST_DIR}/Src"
        PREFIX "User/Src"
        FILES ${USER_C_SOURCES}
    )
endif()


# -----------------------------------------------------------------------------
# 6. CMake配置阶段诊断信息
# -----------------------------------------------------------------------------
#
# 工程稳定后可以删除或注释这些message，不影响编译。
#

message(STATUS "============================================================")
message(STATUS "User source root : ${CMAKE_CURRENT_LIST_DIR}/Src")
message(STATUS "User include root: ${CMAKE_CURRENT_LIST_DIR}/Inc")
message(STATUS "User C sources   : ${USER_C_SOURCES}")
message(STATUS "============================================================")