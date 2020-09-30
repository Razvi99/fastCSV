cmake_minimum_required(VERSION 3.16)
project(fastCSV)

set(CMAKE_CXX_STANDARD 17)

add_compile_options("-march=native")
#set(CMAKE_CXX_FLAGS_DEBUG "-fno-omit-frame-pointer")
#set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fno-omit-frame-pointer")

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

# pthread (required by zlib)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(fastCSV main.cpp)

get_filename_component(BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
target_link_libraries(fastCSV Threads::Threads ${BASE_DIR}/lib/zlib/libz.a)