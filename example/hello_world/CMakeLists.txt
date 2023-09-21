cmake_minimum_required(VERSION 2.8)

project(hello_world)

file(GLOB_RECURSE srcs ${PROJECT_SOURCE_DIR}/src/*.c)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(${PROJECT_NAME} SHARED
    ${srcs}
)