#ifndef SDL_GL_HPP
#define SDL_GL_HPP

#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

class SdlGl {
 public:
  void setViewport(int new_width, int new_height);

  void SDL_die(std::string error);
  void initDisplay(int windowCount);

  std::vector<SDL_Window*> windows;
  SDL_GLContext gl_context;
  unsigned int frame_number = 0;

  int pixel_width, pixel_height;
  float viewspace_width, viewspace_height;
};

#endif
