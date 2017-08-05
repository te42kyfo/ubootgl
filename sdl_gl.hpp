#ifndef SDL_GL_HPP
#define SDL_GL_HPP

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

class SdlGl {
 public:
  void SDL_die(std::string error);
  SdlGl();
  ~SdlGl();
  void initDisplay();
  void deinitDisplay();

  SDL_Window *window;
  SDL_GLContext gl_context;
};

void glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
                   GLsizei length, const GLchar *message,
                   const void *userParam);

#endif
