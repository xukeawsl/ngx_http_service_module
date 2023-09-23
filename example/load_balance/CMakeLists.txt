cmake_minimum_required(VERSION 2.8)

project(load_balance)

file(GLOB_RECURSE srcs ${PROJECT_SOURCE_DIR}/src/*.c)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(${PROJECT_NAME} SHARED
    ${srcs}
)