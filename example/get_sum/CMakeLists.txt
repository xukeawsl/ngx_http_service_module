cmake_minimum_required(VERSION 2.8)

project(get_sum)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(add SHARED src/add.c)
add_library(${PROJECT_NAME} SHARED src/sum.c)