cmake_minimum_required(VERSION 3.10)

project(octo
    VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")


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

message("cxx compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
