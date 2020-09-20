cmake_minimum_required(VERSION 3.16)
project(fastCSV)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

add_executable(fastCSV main.cpp lib/miniz/miniz.cpp)