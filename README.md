# octo readme

This project is intended to draw stuff on screen using as few dependencies as possible.

## build

needs a compiler with some c++20 features supported: source_location (P1208R6), <format> (P0645R10).
fine with clang++-16.

## log

- created a window containing a quad using Xlib & GLX following [this](https://www.khronos.org/opengl/wiki/Programming_OpenGL_in_Linux:_GLX_and_Xlib) guide.

- switched to OpenGL3.0 context creation using [this](https://www.khronos.org/opengl/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)). Turns out we need sth called 'Debug Context'.
