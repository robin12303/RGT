cmake_minimum_required(VERSION 3.20)
project(problem2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(problem2
    main.cpp
    CircularBuffer.hpp
)

target_include_directories(problem2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (MSVC)
  target_compile_options(problem2 PRIVATE /utf-8)
endif()
