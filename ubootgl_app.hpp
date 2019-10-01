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
      : sim("resources/level2.png", 1.0, 0.01f),
        rock_texture("resources/rock_texture2.png") {
    Draw2DBuf::init();
    DrawStreamlines::init();
    DrawTracers::init();
    DrawFloatingItems::init();

    scale = 10.0;

    textures.push_back(Texture("resources/ship2.png"));
    textures.push_back(Texture("resources/debris1.png"));
    textures.push_back(Texture("resources/debris2.png"));
    textures.push_back(Texture("resources/agent2.png"));
    textures.push_back(Texture("resources/torpedo.png"));
    textures.push_back(Texture("resources/explosion.png", 4, 4));
    // textures.push_back(Texture("tex_test3x3.png", 3, 3));


    playerShips.push_back({glm::vec2{0.004, 0.001}, 2.2, glm::vec2(0, 0),
                           glm::vec2(0.55, 0.23), 0.0, 0.0, &textures[0]});
    playerShips.push_back({glm::vec2{0.004, 0.001}, 2.2, glm::vec2(0, 0),
                           glm::vec2(0.45, 0.18), 0.0, 0.0, &textures[0]});
    playerShips.push_back({glm::vec2{0.004, 0.001}, 2.2, glm::vec2(0, 0),
                           glm::vec2(0.145, 0.18), 0.0, 0.0, &textures[0]});

    for (int i = 0; i < 100; i++) {
      swarm.addAgent({glm::vec2{0.0025, 0.001}, 0.5, glm::vec2(0, 0),
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

  std::vector<FloatingItem> playerShips;

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
