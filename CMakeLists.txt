cmake_minimum_required(VERSION 3.18)

project(mySelf C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wno-missing-braces")
add_executable(mySelf
  src/failure.c
  src/hash.c
  src/lexer.c
  src/object.c
  src/parser.c
  src/self.c
  src/stack.c)
