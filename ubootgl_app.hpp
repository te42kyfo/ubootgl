#pragma once

#include <iostream>
#include <vector>
#include <glm/vec2.hpp>
#include "draw_2dbuf.hpp"
#include "draw_streamlines.hpp"
#include "draw_text.hpp"
#include "draw_tracers.hpp"

#include "imgui/imgui.h"
#include "sdl_gl.hpp"
#include "simulation.hpp"
#include "texture.hpp"

class UbootGlApp {
 public:
  UbootGlApp()
      : sim("level2.png", 1.0, 0.01f), rock_texture("rock_texture2.png") {
    Draw2DBuf::init();
    DrawStreamlines::init();
    DrawTracers::init();
    scale = 1.0;
  }

  void loop();
  void draw();
  void handleKey(SDL_KeyboardEvent event);

  double lastFrameTime = 0;
  double smoothedFrameRate = 0;
  float scale;
  float simTime = 0.0;
  uint simIterationCounter = 0;
  SdlGl vis;
  Simulation sim;
  Texture rock_texture;

  glm::vec2 playerPosition = {0, 0};
  double lastKeyUpdate;
  std::vector<bool> keysPressed = std::vector<bool>(4, false);
};
