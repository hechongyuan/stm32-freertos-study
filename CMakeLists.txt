cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME 1.2_key)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
# 注意：这里只创建目标，不写源文件；用户源文件在下面用 GLOB 统一收集
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
# 用 GLOB 自动收集 FreeRTOS 内核源文件（FreeRTOS/Src 目录下所有 .c 文件）
# CONFIGURE_DEPENDS：构建时若发现该目录内容变化，会自动重新执行 CMake 配置
file(GLOB_RECURSE FREERTOS_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Src/*.c
)

# 用户自己写的任务/驱动代码：递归收集 User 目录下所有 .c
file(GLOB_RECURSE USER_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/User/*.c
)

# 用户自己写的驱动代码（如 Drivers/key）
# 注意：Drivers/ 下面还有 STM32F1xx_HAL_Driver 和 CMSIS，
# 那些已经由 cmake/stm32cubemx 子工程编译，这里绝不能递归收集 Drivers/ 全部，
# 否则链接时会报 multiple definition。
file(GLOB_RECURSE Drivers_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/key/*.c
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${FREERTOS_SOURCES}
    ${USER_SOURCES}
    ${Drivers_SOURCES}
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # FreeRTOS 头文件目录（FreeRTOS.h、task.h、portmacro.h 等）
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS/Inc
    # 用户头文件目录（Key_Task.h 在 User/Task 下）
    ${CMAKE_CURRENT_SOURCE_DIR}/User
    ${CMAKE_CURRENT_SOURCE_DIR}/User/Task
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/key
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
