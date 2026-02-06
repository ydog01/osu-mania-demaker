cmake_minimum_required(VERSION 3.10)
project(demaker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++ -s -O2 -DNDEBUG")

add_executable(demaker src/main.cpp)

target_include_directories(demaker PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(demaker PROPERTIES SUFFIX ".exe")