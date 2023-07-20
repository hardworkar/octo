cmake_minimum_required(VERSION 3.10)

project(octo
    VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

configure_file(src/globals.h.in src/globals.h)

add_executable(octo
    src/octo.cpp
    src/application.cpp
    src/platform/x11/opengl_x11.cpp
    )

#!todo some better way to deal with *.h.in files
target_include_directories(octo
    PUBLIC "${PROJECT_BINARY_DIR}/src/")

find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)

target_link_libraries(octo GL)
target_link_libraries(octo GLU)
target_link_libraries(octo X11)
