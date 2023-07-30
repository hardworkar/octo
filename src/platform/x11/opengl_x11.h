#pragma once

#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <memory>
#include <string_view>
#include <vector>

namespace octo {

class OpenGLX11 {
public:
  OpenGLX11();
  void test();
  virtual ~OpenGLX11();

private:
  void create_window_();
  void load_glx_functions_();
  void create_context_();
  void setup_gl_();
  void load_gl_functions_();

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

  GLint opengl_major_;
  GLint opengl_minor_;

  std::vector<std::string> glx_extensions_;
  std::vector<std::string> gl_extensions_;

  // GLX functions
  PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = nullptr;

  // GL functions
  PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback = nullptr;
  PFNGLGETSTRINGIPROC glGetStringi = nullptr;
  PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
  PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
  PFNGLBUFFERDATAPROC glBufferData = nullptr;

  PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
  PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
  PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
  PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;

  PFNGLCREATESHADERPROC glCreateShader = nullptr;
  PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
  PFNGLCOMPILESHADERPROC glCompileShader = nullptr;

  PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
  PFNGLATTACHSHADERPROC glAttachShader = nullptr;
  PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;

  PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
};
} // namespace octo
