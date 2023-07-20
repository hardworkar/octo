#include "opengl_x11.h"
#include <GL/glext.h>
#include <iostream>

OpenGLX11::OpenGLX11() {
  createWindow_();
  setupGL_();
}

void OpenGLX11::createWindow_() {
  if ((dpy_ = XOpenDisplay(nullptr)) == nullptr) {
    std::cerr << "Failed to connect to X server\n";
    exit(-1);
  }

  root_ = DefaultRootWindow(dpy_);

  if ((vi_ = glXChooseVisual(dpy_, 0, att_)) == nullptr) {
    std::cerr << "No appropriate visual found\n";
    exit(-1);
  }
  std::cout << "visual " << std::hex << vi_->visualid << " selected \n";

  cmap_ = XCreateColormap(dpy_, root_, vi_->visual, AllocNone);

  swa_.colormap = cmap_;
  swa_.event_mask = ExposureMask | KeyPressMask;

  win_ = XCreateWindow(dpy_, root_, 0, 0, 600, 600, 0, vi_->depth, InputOutput,
                       vi_->visual, CWColormap | CWEventMask, &swa_);
  XMapWindow(dpy_, win_);
  XStoreName(dpy_, win_, "octo window!");
}

void OpenGLX11::setupGL_() {
  glc_ = glXCreateContext(dpy_, vi_, nullptr, GL_TRUE);
  glXMakeCurrent(dpy_, win_, glc_);

  glEnable(GL_DEPTH_TEST);
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
