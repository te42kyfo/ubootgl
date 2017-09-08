#pragma once

#include <glm/vec2.hpp>
#include <iostream>
#include <random>
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
      : sim("level2.png", 1.0, 0.0001f), rock_texture("rock_texture2.png") {
    Draw2DBuf::init();
    DrawStreamlines::init();
    DrawTracers::init();
    DrawFloatingItems::init();

    scale = 8.0;

    std::default_random_engine gen;
    std::uniform_real_distribution<float> disx(0.0f, 1.0f);
    std::uniform_real_distribution<float> disy(0.0f,
                                               (float)sim.height / sim.width);

    textures.push_back(Texture("ship.png"));
    textures.push_back(Texture("debris1.png"));
    textures.push_back(Texture("debris2.png"));

    for (int i = 0; i < 10000; i++) {
      sim.floatingItems.push_back({glm::vec2(0, 0.0), glm::vec2(0, 0),
                                   glm::vec2(disx(gen), disy(gen)), 1.0,
                                   glm::vec2{0.002, 0.002}, disx(gen) * 3.141f,
                                   &(textures[i % 2 + 1])});
    }

    sim.floatingItems.push_back({glm::vec2(0, 0), glm::vec2(0, 0),
                                 glm::vec2(0.5, 0.21), 1.0,
                                 glm::vec2{0.0004, 0.0011}, 0.0, &textures[0]});
    ship = &sim.floatingItems.back();
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

  FloatingItem* ship;

  double lastKeyUpdate;
  std::vector<bool> keysPressed = std::vector<bool>(6, false);

  std::vector<Texture> textures;
};
