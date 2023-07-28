#include "opengl_x11.h"
#include "../../log.h"
#include <GL/glext.h>
#include <GL/glx.h>
#include <regex>
#include <stdexcept>

namespace octo {

// trapping x11 errors
static int x_trapped_error_code;
static int x_error_handler(Display *dpy, XErrorEvent *ev) {
  x_trapped_error_code = ev->error_code;
  return 0;
}

decltype(&x_error_handler) x_old_error_handler = nullptr;

// Helper to check for extension string presence.  Adapted from:
// http://www.opengl.org/resources/features/OGLextensions/
static bool isExtensionSupported(const std::string &extList,
                                 const std::string &extension) {
  if (extension.length() == 0 || extension.contains(' '))
    return false;
  return std::regex_search(extList, std::regex("\\b" + extension + "\\b"));
}

OpenGLX11::OpenGLX11() {
  create_window_();
  create_context_();
  setup_gl_();
}

void OpenGLX11::create_window_() {
  // Display = server + screens + input devices
  // Screen = typically an independent monitor. linked to the Display

  if (!(dpy_ = XOpenDisplay(NULL)))
    throw std::runtime_error("Failed to connect to X server");

  if (!glXQueryVersion(dpy_, &glx_major_, &glx_minor_) ||
      ((glx_major_ == 1) && (glx_minor_ < 3)) || (glx_major_ < 1)) {
    throw std::runtime_error("Invalid GLX version");
  }

  log(log_level::Info, "Getting matching framebuffer configs");
  static int visual_attribs[] = {
      GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
      GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
      GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True,
      // GLX_SAMPLE_BUFFERS  , 1,
      // GLX_SAMPLES         , 4,
      None};

  int fbcount;
  GLXFBConfig *fbc =
      glXChooseFBConfig(dpy_, DefaultScreen(dpy_), visual_attribs, &fbcount);
  if (!fbc) {
    throw std::runtime_error("Failed to retrieve a framebuffer config");
  }
  log(log_level::Info, std::format("Found {} matching FB configs", fbcount));

  bestFbc_ = fbc[0];
  XFree(fbc);

  // Get a visual
  vi_ = glXGetVisualFromFBConfig(dpy_, bestFbc_);
  log(log_level::Info, std::format("Chosen visual ID = 0x{:x}\n",
                                   XVisualIDFromVisual(vi_->visual)));

  log(log_level::Info, "Creating colormap");
  cmap_ = XCreateColormap(dpy_, RootWindow(dpy_, vi_->screen), vi_->visual,
                          AllocNone);
  swa_.colormap = cmap_;
  swa_.background_pixmap = None;
  swa_.border_pixel = 0;
  swa_.event_mask = StructureNotifyMask;

  log(log_level::Info, "Creating window");
  if (!(win_ =
            XCreateWindow(dpy_, RootWindow(dpy_, vi_->screen), 0, 0, 600, 600,
                          0, vi_->depth, InputOutput, vi_->visual,
                          CWBorderPixel | CWColormap | CWEventMask, &swa_))) {
    throw std::runtime_error("Failed to create window");
  }

  XStoreName(dpy_, win_, "octo!");
  XMapWindow(dpy_, win_);
}

void OpenGLX11::create_context_() {
  // Get the default screen's GLX extension list
  const char *glxExts = glXQueryExtensionsString(dpy_, DefaultScreen(dpy_));

  PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB =
      (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB(
          (const GLubyte *)"glXCreateContextAttribsARB");

  x_trapped_error_code = 0;
  x_old_error_handler = XSetErrorHandler(x_error_handler);
  // Check for the GLX_ARB_create_context extension string and the function.
  // If either is not present, use GLX 1.3 context creation method.
  if (!isExtensionSupported(glxExts, "GLX_ARB_create_context") ||
      !glXCreateContextAttribsARB) {
    log(log_level::Info, "glXCreateContextAttribsARB() not found");
    ctx_ = glXCreateNewContext(dpy_, bestFbc_, GLX_RGBA_TYPE, 0, True);
  } else {
    int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                             GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                             GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
                             // GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                             None};

    log(log_level::Info, "Creating context");
    ctx_ = glXCreateContextAttribsARB(dpy_, bestFbc_, 0, True, context_attribs);

    // Sync to ensure any errors generated are processed.
    XSync(dpy_, False);
    if (!x_trapped_error_code && ctx_)
      log(log_level::Info, "Created GL 3.0 context");
    else {
      // Couldn't create GL 3.0 context.  Fall back to old-style 2.x context.
      // When a context version below 3.0 is requested, implementations will
      // return the newest context version compatible with OpenGL versions less
      // than version 3.0.
      // GLX_CONTEXT_MAJOR_VERSION_ARB = 1
      context_attribs[1] = 1;
      // GLX_CONTEXT_MINOR_VERSION_ARB = 0
      context_attribs[3] = 0;

      x_trapped_error_code = 0;

      log(log_level::Info, "Failed to create GL 3.0 context");
      ctx_ =
          glXCreateContextAttribsARB(dpy_, bestFbc_, 0, True, context_attribs);
    }
  }

  // Sync to ensure any errors generated are processed.
  XSync(dpy_, False);

  XSetErrorHandler(x_old_error_handler);
  if (x_trapped_error_code || !ctx_) {
    throw std::runtime_error("Failed to create an OpenGL context");
  }

  // Verifying that context is a direct context
  if (!glXIsDirect(dpy_, ctx_)) {
    log(log_level::Info, "Indirect GLX rendering context obtained");
  } else {
    log(log_level::Info, "Direct GLX rendering context obtained");
  }
}

void OpenGLX11::setup_gl_() {
  glXMakeCurrent(dpy_, win_, ctx_);

  glEnable(GL_DEPTH_TEST);
}

OpenGLX11::~OpenGLX11() {
  XDestroyWindow(dpy_, win_);
  XFreeColormap(dpy_, cmap_);
  XFree(vi_);
  XCloseDisplay(dpy_);
}

void OpenGLX11::test() {
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1., 1., -1., 1., 1., 20.);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

  glBegin(GL_QUADS);
  glColor3f(1., 0., 0.);
  glVertex3f(-.75, -.75, 0.);
  glColor3f(0., 1., 0.);
  glVertex3f(.75, -.75, 0.);
  glColor3f(0., 0., 1.);
  glVertex3f(.75, .75, 0.);
  glColor3f(1., 1., 0.);
  glVertex3f(-.75, .75, 0.);
  glEnd();

  glXSwapBuffers(dpy_, win_);
}
} // namespace octo
