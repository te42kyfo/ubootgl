#include "sdl_gl.hpp"
#include <iostream>
#include "gl_error.hpp"

using namespace std;

void SdlGl::setViewport(int new_width, int new_height) {
  pixel_width = new_width;
  pixel_height = new_height;

  GL_CALL(glViewport(0, 0, (GLsizei)pixel_width, (GLsizei)pixel_height));
}

void SdlGl::SDL_die(string error) {
  cout << "Fatal error in " << error << ": " << SDL_GetError() << "\n";
  exit(EXIT_FAILURE);
}

void SdlGl::initDisplay(int windowCount) {
  if (windowCount < 1) return;
  if (SDL_Init(SDL_INIT_VIDEO) == -1) SDL_die("SDL_Init");

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

  for (int i = 0; i < windowCount; i++) {
    windows.push_back(SDL_CreateWindow(
        "WAVE 1", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE));
    if (windows.back() == nullptr) SDL_die("SDL_CreateWindow");
  }

  gl_context = SDL_GL_CreateContext(windows[0]);
  if (gl_context == nullptr) SDL_die("SDL_GL_CreateContext");

  SDL_GL_SetSwapInterval(1);

  glewExperimental = GL_TRUE;
  glewInit();
  GL_CALL();
  cout << glGetString(GL_VENDOR) << "\n";
  cout << glGetString(GL_RENDERER) << "\n";
  cout << glGetString(GL_VERSION) << "\n";
  cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";
}
