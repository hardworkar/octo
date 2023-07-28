#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <memory>

namespace octo {

class OpenGLX11 {
public:
  OpenGLX11();
  void test();
  virtual ~OpenGLX11();

private:
  void create_window_();
  void create_context_();
  void setup_gl_();

private:
  Display *dpy_;
  GLXFBConfig bestFbc_;
  XVisualInfo *vi_;
  Window root_;
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
