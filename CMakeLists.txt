cmake_minimum_required(VERSION 3.20)

project(markdown_parser_c C)

set(CMAKE_C_STANDARD 11)

add_executable(markdown_parser_c src/main.c)

target_link_libraries(markdown_parser_c PRIVATE regex)
