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

    textures.emplace(registry.type<entt::tag<"tex_ship"_hs>>(),
                     Texture("resources/ship2.png"));
    textures.emplace(registry.type<entt::tag<"tex_debris1"_hs>>(),
                     Texture("resources/debris1.png"));
    textures.emplace(registry.type<entt::tag<"tex_debris2"_hs>>(),
                     Texture("resources/debris2.png"));
    textures.emplace(registry.type<entt::tag<"tex_agent"_hs>>(),
                     Texture("resources/agent2.png"));
    textures.emplace(registry.type<entt::tag<"tex_torpedo"_hs>>(),
                     Texture("resources/torpedo.png"));
    textures.emplace(registry.type<entt::tag<"tex_explosion"_hs>>(),
                     Texture("resources/explosion.png", 4, 4));

    for (int i = 0; i < 2; i++) {
      auto newAgent = registry.create();
      registry.assign<CoItem>(newAgent, glm::vec2{0.0025f, 0.001f},
                              glm::vec2(0.2f + i * 0.001f, 0.2f), 0.0f);
      registry.assign<entt::tag<"tex_agent"_hs>>(newAgent);
      registry.assign<CoAgent>(newAgent);
      registry.assign<CoKinematics>(newAgent, 0.5, glm::vec2(0.0f, 0.0f), 0.0f);
      registry.assign<CoRespawnsOoB>(newAgent);
      registry.assign<CoTarget>(newAgent);
    }
  }

  void loop();
  void draw();
  void handleKey(SDL_KeyboardEvent event);

  void processTorpedos();
  void processExplosions();

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
  Texture black_texture = Texture("resources/black.png");


  double lastKeyUpdate;

  entt::registry registry;
  std::map<SDL_Keycode, bool> keysPressed;
  std::map<entt::component, Texture> textures;
};
