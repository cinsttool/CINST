## 编译当前项目依赖的 cmake 最低版本
cmake_minimum_required(VERSION 3.0.0)
## 项目名
project(native)
if(NOT DEFINED ENV{JAVA_HOME})
    message(FATEL_ERROR "not defined environment variable:JAVA_HOME")
endif()
## 添加依赖目录（之前复制到项目目录下的 include 及其子目录）
include_directories($ENV{JAVA_HOME}/include)
include_directories($ENV{JAVA_HOME}/include/linux)
include_directories(./)

## 声明要构建为动态链接库：agent.cpp是我们编写的 agent 主程序，SHARED 表示动态链接库
add_library(native SHARED native.cpp)

install(TARGETS native
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include
)