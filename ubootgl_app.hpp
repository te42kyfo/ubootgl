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
#include <map>
#include <random>
#include <vector>

class UbootGlApp {
public:
  UbootGlApp()
      : sim("level2.png", 1.0, 0.00001f), rock_texture("rock_texture2.png") {
    Draw2DBuf::init();
    DrawStreamlines::init();
    DrawTracers::init();
    DrawFloatingItems::init();

    scale = 6.0;

    textures.push_back(Texture("ship2.png"));
    textures.push_back(Texture("debris1.png"));
    textures.push_back(Texture("debris2.png"));
    textures.push_back(Texture("agent2.png"));
    textures.push_back(Texture("torpedo.png"));
    textures.push_back(Texture("explosion.png", 4, 4));
    // textures.push_back(Texture("tex_test3x3.png", 3, 3));

    for (int i = 0; i < 1000; i++) {
      float size = (0.1f + rand() % 100 / 50.0f);
      debris.push_back(
          {glm::vec2{0.001, 0.001} * (0.1f + rand() % 100 / 100.0f), size,
           glm::vec2(0, 0), glm::vec2(-1.0, -1.0),
           rand() % 100 / 100.0f * 2.0f * (float)M_PI, 0.0,
           &(textures[i % 2 + 1])});
    }

    ship = {glm::vec2{0.001, 0.004},
            2.2,
            glm::vec2(0, 0),
            glm::vec2(0.5, 0.21),
            0.0,
            0.0,
            &textures[0]};

    for (int i = 0; i < 100; i++) {
      swarm.addAgent({glm::vec2{0.001, 0.0025}, 0.5, glm::vec2(0, 0),
                      glm::vec2(-1.0, -1.0), 0.0, 0.0, &(textures[3])});
    }
    swarm.nnInit();
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
  std::vector<Torpedo> torpedos;
  std::vector<FloatingItem> explosions;
  Swarm swarm;

  double lastKeyUpdate;
  // std::vector<bool> keysPressed = std::vector<bool>(6, false);

  std::map<SDL_Keycode, bool> keysPressed = {
      {SDLK_RIGHT, false}, {SDLK_LEFT, false}, {SDLK_UP, false},
      {SDLK_DOWN, false},  {SDLK_PLUS, false}, {SDLK_MINUS, false},
      {SDLK_SPACE, false}};
  std::vector<Texture> textures;
};
