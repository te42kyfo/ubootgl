#pragma once

#include <glm/vec2.hpp>
#include <iostream>
#include <vector>
#include "draw_2dbuf.hpp"
#include "draw_floating_items.hpp"
#include "draw_streamlines.hpp"
#include "draw_tracers.hpp"
#include "floating_item.hpp"
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
    DrawFloatingItems::init();
    scale = 6.0;

    sim.floatingItems.push_back({glm::vec2(0, 0), glm::vec2(0, 0),
                                 glm::vec2(0.0, 0.005), 1.0,
                                 glm::vec2{0.005, 0.005}, 0.0 * 3.141});

    sim.floatingItems.push_back({glm::vec2(0, 0), glm::vec2(0, 0),
                                 glm::vec2(0.005, 0.0), 1.0,
                                 glm::vec2{0.005, 0.005}, 0.5 * 3.141});
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
