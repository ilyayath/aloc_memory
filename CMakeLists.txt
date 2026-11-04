cmake_minimum_required(VERSION 3.28)
project(aloc_memory C)

set(CMAKE_C_STANDARD 11)

add_library(osmem SHARED src/osmem.c utils/printf.c)
target_include_directories(osmem PRIVATE utils)
target_compile_options(osmem PRIVATE -Wall -Wextra)
