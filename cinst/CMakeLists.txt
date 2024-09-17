## 编译当前项目依赖的 cmake 最低版本
cmake_minimum_required(VERSION 3.0.0)
## 项目名
project(CINST)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")
set(ENABLE_CSV_OUTPUT OFF CACHE BOOL "choose ON to enable csv format output")
set(CMAKE_CXX_FLAGS "-O3 ${CMAKE_CXX_FLAGS}")
if (${ENABLE_CSV_OUTPUT} STREQUAL ON)
    add_compile_definitions(__CSV_OUTPUT__)
endif()

find_package(spdlog REQUIRED)

if (NOT ${spdlog_FOUND})
    message(FATAL "spdlog not found")
endif()
include_directories(${spdlog_INCLUDE_DIR})
add_library(spdlog STATIC IMPORTED)

add_subdirectory(agent)
add_subdirectory(native)
add_subdirectory(preload)
add_subdirectory(scripts)
