#pragma once

#include "draw_2dbuf.hpp"
#include "draw_floating_items.hpp"
#include "draw_streamlines.hpp"
#include "draw_tracers.hpp"
#include "floating_item.hpp"
#include "imgui/imgui.h"
#include "sdl_gl.hpp"
#include "simulation.hpp"
#include "swarm.hpp"
#include "texture.hpp"
#include <glm/vec2.hpp>
#include <iostream>
#include <random>
#include <vector>

class UbootGlApp {
public:
  UbootGlApp()
      : sim("level2.png", 1.0, 0.001f), rock_texture("rock_texture2.png") {
    Draw2DBuf::init();
    DrawStreamlines::init();
    DrawTracers::init();
    DrawFloatingItems::init();

    scale = 8.0;

    textures.push_back(Texture("ship.png"));
    textures.push_back(Texture("debris1.png"));
    textures.push_back(Texture("debris2.png"));
    textures.push_back(Texture("agent.png"));

    for (int i = 0; i < 100; i++) {
      debris.push_back({glm::vec2{0.002, 0.002}, -0.8f + 2.0f * (i % 2 + 1),
                        glm::vec2(0, 0), glm::vec2(-1.0, -1.0), 0.0, 0.0,
                        &(textures[i % 2 + 1])});
    }

    ship = {glm::vec2{0.001, 0.003},
            1.0,
            glm::vec2(0, 0),
            glm::vec2(0.5, 0.21),
            0.0,
            0.0,
            &textures[0]};

    for (int i = 0; i < 100; i++) {
      swarm.addAgent({glm::vec2{0.002, 0.002}, 0.5, glm::vec2(0, 0),
                      glm::vec2(-1.0, -1.0), 0.0, 0.0, &(textures[3])});
    }
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

  FloatingItem ship;
  std::vector<FloatingItem> debris;
  Swarm swarm;

  double lastKeyUpdate;
  std::vector<bool> keysPressed = std::vector<bool>(6, false);

  std::vector<Texture> textures;
};
