cmake_minimum_required(VERSION 3.30)
project(LiveOrigin C)

set(CMAKE_C_STANDARD 11)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(lib/cJSON)
add_subdirectory(lib/libmaxminddb)

file(GLOB SOURCE_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.c
        ${CMAKE_CURRENT_SOURCE_DIR}/ws/*.h ${CMAKE_CURRENT_SOURCE_DIR}/ws/*.c)

add_executable(LiveOrigin ${SOURCE_FILE})
target_include_directories(LiveOrigin PRIVATE
    /usr/include
    lib/cJSON
    lib/libmaxminddb/include)
target_link_libraries(LiveOrigin PRIVATE cjson
    pcap 
    curl 
    maxminddb)
