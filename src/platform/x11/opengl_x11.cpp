#include "opengl_x11.h"
#include "../../log.h"
#include <GL/glext.h>
#include <X11/Xlib.h>
#include <regex>
#include <stdexcept>

namespace octo {

// Helper to check for extension string presence.  Adapted from:
// http://www.opengl.org/resources/features/OGLextensions/
static bool isExtensionSupported(const std::string &extList,
                                 const std::string &extension) {
  if (extension.length() == 0 || extension.contains(' '))
    return false;
  const std::regex extRegex("\\b" + extension + "\\b");
  return std::regex_search(extList, extRegex);
}

static bool ctxErrorOccurred = false;
static int ctxErrorHandler(Display *dpy, XErrorEvent *ev) {
  ctxErrorOccurred = true;
  return 0;
}

OpenGLX11::OpenGLX11() {
  create_window_();
  setup_gl_();
}

void OpenGLX11::create_window_() {
  if ((dpy_ = XOpenDisplay(nullptr)) == nullptr) {
    throw std::runtime_error("Failed to connect to X server");
  }
  // Get a matching FB config
  static int visual_attribs[] = {
      GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
      GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
      GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True,
      // GLX_SAMPLE_BUFFERS  , 1,
      // GLX_SAMPLES         , 4,
      None};

  // FBConfigs were added in GLX version 1.3.
  if (!glXQueryVersion(dpy_, &glx_major_, &glx_minor_) ||
      ((glx_major_ == 1) && (glx_minor_ < 3)) || (glx_major_ < 1)) {
    throw std::runtime_error("Invalid GLX version");
  }

  log(log_level::Info, "Getting matching framebuffer configs");
  int fbcount;
  GLXFBConfig *fbc =
      glXChooseFBConfig(dpy_, DefaultScreen(dpy_), visual_attribs, &fbcount);
  if (!fbc) {
    throw std::runtime_error("Failed to retrieve a framebuffer config");
  }
  log(log_level::Info, std::format("Found {} matching FB configs", fbcount));

  // Pick the FB config/visual with the most samples per pixel
  log(log_level::Info, "Getting XVisualInfos");
  int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;

  for (int i = 0; i < fbcount; ++i) {
    XVisualInfo *vi = glXGetVisualFromFBConfig(dpy_, fbc[i]);
    if (vi) {
      int samp_buf, samples;
      glXGetFBConfigAttrib(dpy_, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
      glXGetFBConfigAttrib(dpy_, fbc[i], GLX_SAMPLES, &samples);

      log(log_level::Info,
          std::format("Matching fbconfig {}, visual ID 0x{:x}: "
                      "SAMPLE_BUFFERS = {}, SAMPLES = {}",
                      i, vi->visualid, samp_buf, samples));

      if (best_fbc < 0 || samp_buf && samples > best_num_samp)
        best_fbc = i, best_num_samp = samples;
      if (worst_fbc < 0 || !samp_buf || samples < worst_num_samp)
        worst_fbc = i, worst_num_samp = samples;
    }
    XFree(vi_);
  }

  GLXFBConfig bestFbc = fbc[best_fbc];

  // Be sure to free the FBConfig list allocated by glXChooseFBConfig()
  XFree(fbc);

  // Get a visual
  vi_ = glXGetVisualFromFBConfig(dpy_, bestFbc);
  log(log_level::Info,
      std::format("Chosen visual ID = 0x{:x}\n", vi_->visualid));

  log(log_level::Info, "Creating colormap");
  XSetWindowAttributes swa;
  Colormap cmap;
  swa.colormap = cmap = XCreateColormap(dpy_, RootWindow(dpy_, vi_->screen),
                                        vi_->visual, AllocNone);
  swa.background_pixmap = None;
  swa.border_pixel = 0;
  swa.event_mask = StructureNotifyMask;

  log(log_level::Info, "Creating window");
  win_ = XCreateWindow(dpy_, RootWindow(dpy_, vi_->screen), 0, 0, 600, 600, 0,
                       vi_->depth, InputOutput, vi_->visual,
                       CWBorderPixel | CWColormap | CWEventMask, &swa);
  if (!win_) {
    throw std::runtime_error("Failed to create window");
    exit(1);
  }

  // Done with the visual info data
  XFree(vi_);

  XStoreName(dpy_, win_, "GL 3.0 Window");

  log(log_level::Info, "Mapping window");
  XMapWindow(dpy_, win_);

  // Get the default screen's GLX extension list
  const char *glxExts = glXQueryExtensionsString(dpy_, DefaultScreen(dpy_));

  // NOTE: It is not necessary to create or make current to a context before
  // calling glXGetProcAddressARB
  PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB =
      (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB(
          (const GLubyte *)"glXCreateContextAttribsARB");

  // Install an X error handler so the application won't exit if GL 3.0
  // context allocation fails.
  //
  // Note this error handler is global.  All dpy_ connections in all threads
  // of a process use the same error handler, so be sure to guard against other
  // threads issuing X commands while this code is running.
  ctxErrorOccurred = false;
  int (*oldHandler)(Display *, XErrorEvent *) =
      XSetErrorHandler(&ctxErrorHandler);

  // Check for the GLX_ARB_create_context extension string and the function.
  // If either is not present, use GLX 1.3 context creation method.
  if (!isExtensionSupported(glxExts, "GLX_ARB_create_context") ||
      !glXCreateContextAttribsARB) {
    log(log_level::Info, "glXCreateContextAttribsARB() not found"
                         " ... using old-style GLX context");
    ctx_ = glXCreateNewContext(dpy_, bestFbc, GLX_RGBA_TYPE, 0, True);
  }

  // If it does, try to get a GL 3.0 context!
  else {
    int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                             GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                             GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
                             // GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                             None};

    log(log_level::Info, "Creating context");
    ctx_ = glXCreateContextAttribsARB(dpy_, bestFbc, 0, True, context_attribs);

    // Sync to ensure any errors generated are processed.
    XSync(dpy_, False);
    if (!ctxErrorOccurred && ctx_)
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

      ctxErrorOccurred = false;

      log(log_level::Info, "Failed to create GL 3.0 context"
                           " ... using old-style GLX context");
      ctx_ =
          glXCreateContextAttribsARB(dpy_, bestFbc, 0, True, context_attribs);
    }
  }

  // Sync to ensure any errors generated are processed.
  XSync(dpy_, False);

  // Restore the original error handler
  XSetErrorHandler(oldHandler);

  if (ctxErrorOccurred || !ctx_) {
    throw std::runtime_error("Failed to create an OpenGL context");
    exit(-1);
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

OpenGLX11::~OpenGLX11() { XCloseDisplay(dpy_); }

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
