cmake_minimum_required(VERSION 3.24.2)

project(ipcfun CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(etc/build_type.cmake)
include(etc/cflags.cmake)
include(etc/scanners.cmake)

add_subdirectory(src)

