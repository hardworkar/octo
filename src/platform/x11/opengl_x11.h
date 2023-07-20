#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>

class OpenGLX11 {
public:
  OpenGLX11();
  void test();

private:
  void createWindow_();
  void setupGL_();

private:
  Display *dpy_;
  Window root_;
  GLint att_[5] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
  XVisualInfo *vi_;
  Colormap cmap_;
  XSetWindowAttributes swa_;
  Window win_;
  GLXContext glc_;
  XWindowAttributes gwa_;
  XEvent xev_;
};
