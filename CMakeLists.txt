cmake_minimum_required(VERSION 3.23)
project(json C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

add_executable(json main.c json_t.c json_t.h util.h parser.c parser.h json_object.c)

enable_testing()

add_executable(json_test test.c json_t.c json_t.h util.h parser.c parser.h json_object.c)
add_test(NAME json_test COMMAND json_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )