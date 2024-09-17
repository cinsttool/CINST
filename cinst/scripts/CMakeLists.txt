cmake_minimum_required(VERSION 3.0.0)

project(scripts)

add_subdirectory(sqlite)

include_directories(sqlite)
include_directories(../native)
include_directories(include)

add_executable(csv2db src/csv2db.cpp)

add_executable(addr2id src/addr2id.cpp )

# add_executable(build_container_db src/build_container_db.cpp)

add_library(fs_utils SHARED src/fs_utils.cpp)

target_link_libraries(csv2db sqlite3)
target_link_libraries(csv2db stdc++fs)
target_link_libraries(csv2db fs_utils)
target_link_libraries(addr2id fs_utils)



install(TARGETS csv2db addr2id fs_utils
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)
