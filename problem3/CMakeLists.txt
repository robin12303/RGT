cmake_minimum_required(VERSION 3.20)
project(problem3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(problem3
  main.cpp
)

target_include_directories(problem3 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (MSVC)
  target_compile_options(problem3 PRIVATE /utf-8)
endif()
