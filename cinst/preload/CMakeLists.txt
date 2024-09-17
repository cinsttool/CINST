## 编译当前项目依赖的 cmake 最低版本
cmake_minimum_required(VERSION 3.0.0)
## 项目名
project(preload)

include_directories(../native)
## 声明要构建为动态链接库：agent.cpp是我们编写的 agent 主程序，SHARED 表示动态链接库
add_library(preload SHARED preload.cpp)
target_link_libraries(preload dl)

install(TARGETS preload
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        PUBLIC_HEADER DESTINATION include
)