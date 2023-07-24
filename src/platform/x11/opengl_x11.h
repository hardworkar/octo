#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>

namespace octo {
class OpenGLX11 {
public:
  OpenGLX11();
  void test();
  virtual ~OpenGLX11();

private:
  void create_window_();
  void setup_gl_();

private:
  Display *dpy_;
  Window root_;
  GLint att_[5] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
  XVisualInfo *vi_;
  Colormap cmap_;
  XSetWindowAttributes swa_;
  Window win_;
  GLXContext ctx_ = 0;
  XWindowAttributes gwa_;
  XEvent xev_;

  int glx_major_;
  int glx_minor_;
};
} // namespace octo
