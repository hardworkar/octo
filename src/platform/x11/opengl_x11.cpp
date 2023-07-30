#include "opengl_x11.h"
#include "../../log.h"
#include <stdexcept>

namespace octo {

// trapping x11 errors
static int x_trapped_error_code;
static int x_error_handler(Display *dpy, XErrorEvent *ev) {
  x_trapped_error_code = ev->error_code;
  return 0;
}

decltype(&x_error_handler) x_old_error_handler = nullptr;

OpenGLX11::OpenGLX11() {
  create_window_();
  load_glx_functions_();
  create_context_();
  setup_gl_();
}

void OpenGLX11::create_window_() {
  if (!(dpy_ = XOpenDisplay(NULL))) {
    throw std::runtime_error("Failed to connect to X server");
  }

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

void OpenGLX11::load_glx_functions_() {
  // Get the default screen's GLX extension list
  const char *glxExts = glXQueryExtensionsString(dpy_, DefaultScreen(dpy_));
  auto split_by_space = [](std::string &&s) {
    std::string delimiter = " ";

    size_t pos = 0;
    std::vector<std::string> tokens;
    while ((pos = s.find(delimiter)) != std::string::npos) {
      tokens.push_back(s.substr(0, pos));
      s.erase(0, pos + delimiter.length());
    }
    return tokens;
  };
  glx_extensions_ = split_by_space(std::string(glxExts));

  glXCreateContextAttribsARB =
      (decltype(glXCreateContextAttribsARB))glXGetProcAddressARB(
          (const GLubyte *)"glXCreateContextAttribsARB");

  auto has_glx_extension_ = [this](const char *extension) {
    return std::find(glx_extensions_.cbegin(), glx_extensions_.cend(),
                     extension) != glx_extensions_.cend();
  };
  if (!has_glx_extension_("GLX_ARB_create_context")) {
    throw std::runtime_error("glXCreateContextAttribsARB() not found");
  }
}

void OpenGLX11::load_gl_functions_() {
  auto load = [](const char *s) {
    return glXGetProcAddressARB((const GLubyte *)s);
  };

  glGetStringi = (decltype(glGetStringi))load("glGetStringi");

  GLint gl_num_extensions;
  glGetIntegerv(GL_NUM_EXTENSIONS, &gl_num_extensions);

  for (GLint i = 0; i < gl_num_extensions; ++i) {
    gl_extensions_.push_back((char *)glGetStringi(GL_EXTENSIONS, i));
  }

  // TODO for some reason it works but our opengl is 4.1
  glDebugMessageCallback =
      (decltype(glDebugMessageCallback))load("glDebugMessageCallback");

  glGenBuffers = (decltype(glGenBuffers))load("glGenBuffers");
  glBindBuffer = (decltype(glBindBuffer))load("glBindBuffer");
  glBufferData = (decltype(glBufferData))load("glBufferData");

  glGenVertexArrays = (decltype(glGenVertexArrays))load("glGenVertexArrays");
  glBindVertexArray = (decltype(glBindVertexArray))load("glBindVertexArray");
  glEnableVertexAttribArray =
      (decltype(glEnableVertexAttribArray))load("glEnableVertexAttribArray");
  glVertexAttribPointer =
      (decltype(glVertexAttribPointer))load("glVertexAttribPointer");

  glCreateShader = (decltype(glCreateShader))load("glCreateShader");
  glShaderSource = (decltype(glShaderSource))load("glShaderSource");
  glCompileShader = (decltype(glCompileShader))load("glCompileShader");

  glCreateProgram = (decltype(glCreateProgram))load("glCreateProgram");
  glAttachShader = (decltype(glAttachShader))load("glAttachShader");
  glLinkProgram = (decltype(glLinkProgram))load("glLinkProgram");

  glUseProgram = (decltype(glUseProgram))load("glUseProgram");
}

void OpenGLX11::create_context_() {
  int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
                           GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                           GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
                           // GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                           None};

  x_trapped_error_code = 0;
  x_old_error_handler = XSetErrorHandler(x_error_handler);

  log(log_level::Info, "Creating context");
  ctx_ = glXCreateContextAttribsARB(dpy_, bestFbc_, 0, True, context_attribs);

  // Sync to ensure any errors generated are processed.
  XSync(dpy_, False);
  XSetErrorHandler(x_old_error_handler);
  if (x_trapped_error_code || !ctx_)
    throw std::runtime_error("Failed to create OpenGL context");

  if (!glXIsDirect(dpy_, ctx_)) {
    log(log_level::Info, "Indirect GLX rendering context obtained");
  } else {
    log(log_level::Info, "Direct GLX rendering context obtained");
  }
}

static void APIENTRY gl_message_callback(GLenum source, GLenum type, GLuint id,
                                         GLenum severity, GLsizei length,
                                         const GLchar *message,
                                         const void *userParam) {
  throw std::runtime_error(message);
}

void OpenGLX11::setup_gl_() {
  glXMakeCurrent(dpy_, win_, ctx_);
  glGetIntegerv(GL_MAJOR_VERSION, &opengl_major_);
  glGetIntegerv(GL_MINOR_VERSION, &opengl_minor_);
  log(log_level::Info,
      std::format("OpenGL version {}.{}", opengl_major_, opengl_minor_));

  load_gl_functions_();

  glDebugMessageCallback(gl_message_callback, 0);

  glEnable(GL_DEPTH_TEST);
}

OpenGLX11::~OpenGLX11() {
  XDestroyWindow(dpy_, win_);
  XFreeColormap(dpy_, cmap_);
  XFree(vi_);
  XCloseDisplay(dpy_);
}

void OpenGLX11::test() {
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  float points[] = {0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, -0.5f, -0.5f, 0.0f};

  GLuint vbo = 0;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, 9 * sizeof(float), points, GL_STATIC_DRAW);

  GLuint vao = 0;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

  const char *vertex_shader = "#version 400\n"
                              "in vec3 vp;"
                              "void main() {"
                              "  gl_Position = vec4(vp, 1.0);"
                              "}";
  const char *fragment_shader = "#version 400\n"
                                "out vec4 frag_colour;"
                                "void main() {"
                                "  frag_colour = vec4(0.0, 1.0, 0.0, 1.0);"
                                "}";
  GLuint vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vertex_shader, NULL);
  glCompileShader(vs);
  GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &fragment_shader, NULL);
  glCompileShader(fs);

  GLuint shader_programme = glCreateProgram();
  glAttachShader(shader_programme, fs);
  glAttachShader(shader_programme, vs);
  glLinkProgram(shader_programme);

  glUseProgram(shader_programme);
  glBindVertexArray(vao);
  // draw points 0-3 from the currently bound VAO with current in-use shader
  glDrawArrays(GL_TRIANGLES, 0, 3);

  glXSwapBuffers(dpy_, win_);
}
} // namespace octo
