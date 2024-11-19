cmake_minimum_required(VERSION 3.10)

project(TestProject)

set(CMAKE_C_STANDARD 11)

add_executable(test bbd.c stun.c message.c)
