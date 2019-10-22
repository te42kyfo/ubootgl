#pragma once
#include "draw_2dbuf.hpp"
#include "draw_floating_items.hpp"
#include "draw_streamlines.hpp"
#include "draw_tracers.hpp"
#include "entt/entity/helper.hpp"
#include "entt/entity/registry.hpp"
#include "floating_item.hpp"
#include "frame_times.hpp"
#include "imgui/imgui.h"
#include "player.hpp"
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
      : sim("resources/level2.png", 0.8, 0.01f),
        rock_texture("resources/rock_texture2.png") {
    Draw2DBuf::init();
    DrawStreamlines::init();
    DrawTracers::init();
    DrawFloatingItems::init();

    scale = 3.0;

    textures.push_back(Texture("resources/ship2.png"));
    textures.push_back(Texture("resources/debris1.png"));
    textures.push_back(Texture("resources/debris2.png"));
    textures.push_back(Texture("resources/agent2.png"));
    textures.push_back(Texture("resources/torpedo.png"));
    textures.push_back(Texture("resources/explosion.png", 4, 4));
    textures.push_back(Texture("resources/black.png"));

    for (int i = 0; i < 2000; i++) {
      auto newAgent = registry.create();
      registry.assign<CoItem>(newAgent, glm::vec2{0.0025f, 0.001f},
                              glm::vec2(0.2f+i*0.001f, 0.2f), 0.0f);
      registry.assign<CoSprite>(newAgent, &textures[3], 0.0f);
      registry.assign<CoAgent>(newAgent);
      registry.assign<CoKinematics>(newAgent, 0.5, glm::vec2(0.0f, 0.0f), 0.0f);
      registry.assign<entt::tag<"agent_tex"_hs>>(newAgent);

      // swarm.addAgent(, 0.5, glm::vec2(0, 0),
      //             glm::vec2(-1.0, -1.0), 0.0, 0.0, &(textures[3])});
    }

  }

  void loop();
  void draw();
  void handleKey(SDL_KeyboardEvent event);

  double lastFrameTime = 0;
  double smoothedFrameRate = 0;
  FrameTimes frameTimes;
  FrameTimes gfxTimes;

  double lastSimulationTime = 1.0;
  float scale;
  float simTime = 0.0;
  int simulationSteps = 0;
  SdlGl vis;
  Simulation sim;
  Texture rock_texture;

  std::vector<FloatingItem> playerShips;
  std::vector<Player> players;
  unsigned int playerCount = 1;

  std::vector<FloatingItem> debris;
  std::vector<Torpedo> torpedos;
  std::vector<FloatingItem> explosions;


  double lastKeyUpdate;

  entt::registry registry;
  std::map<SDL_Keycode, bool> keysPressed;
  std::vector<Texture> textures;
};
