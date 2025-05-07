cmake_minimum_required(VERSION 3.7.3)
project(Clion C)

set(CMAKE_C_STANDARD 99)

add_executable(Clion main.c
        main.h
        keyValStore.c
        keyValStore.h
        sub.h
        sub.c)
