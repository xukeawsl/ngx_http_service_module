cmake_minimum_required(VERSION 2.8)

project(third_test)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(use_third SHARED src/jsoncpp_use.cpp)