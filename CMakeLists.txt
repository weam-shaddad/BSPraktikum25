cmake_minimum_required(VERSION 3.7.2)
project(PrakBS21 C)

set(CMAKE_C_STANDARD 99)

# List only source files; header files will be included automatically.
add_executable(PrakBS21 main.c keyValStore.c sub.c)

# Link the real-time library that provides shm_open and shm_unlink
target_link_libraries(PrakBS21 rt)
