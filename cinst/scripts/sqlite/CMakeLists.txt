cmake_minimum_required(VERSION 3.0.0)
project(sqlite3)

add_library(sqlite3 SHARED sqlite3.c)
target_link_libraries(sqlite3 pthread dl)

install(TARGETS sqlite3
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)