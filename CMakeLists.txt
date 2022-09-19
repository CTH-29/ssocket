cmake_minimum_required(VERSION 2.8)

project(ssocket C)

# include_directories(./src)

file(GLOB SRC_LISTS ./src/*.c)

add_executable(ssocket ${SRC_LISTS})
