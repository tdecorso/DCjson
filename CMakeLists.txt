cmake_minimum_required(VERSION 3.16)

project(json LANGUAGES C VERSION 0.1)

include_directories(include)

set(SRC_FILES src/DCjson.c)

add_executable(test_parsing tests/test_parsing.c ${SRC_FILES})

enable_testing()

add_test(test_parsing test_parsing)