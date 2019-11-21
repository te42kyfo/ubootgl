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
                     Texture("resources/ship3.png", 2, 1));
    textures.emplace(registry.type<entt::tag<"tex_debris1"_hs>>(),
                     Texture("resources/debris1.png"));
    textures.emplace(registry.type<entt::tag<"tex_debris2"_hs>>(),
                     Texture("resources/debris2.png"));
    textures.emplace(registry.type<entt::tag<"tex_debris"_hs>>(),
                     Texture("resources/debris.png", 2, 1));
    textures.emplace(registry.type<entt::tag<"tex_agent"_hs>>(),
                     Texture("resources/agent2.png"));
    textures.emplace(registry.type<entt::tag<"tex_torpedo"_hs>>(),
                     Texture("resources/torpedo.png"));
    textures.emplace(registry.type<entt::tag<"tex_explosion"_hs>>(),
                     Texture("resources/explosion.png", 4, 4));

    for (int i = 0; i < 4; i++) {
      auto newAgent = registry.create();
      registry.assign<CoItem>(newAgent, glm::vec2{0.005f, 0.002f},
                              glm::vec2(0.2f + i * 0.001f, 0.2f), 0.0f);
      registry.assign<entt::tag<"tex_agent"_hs>>(newAgent);
      registry.assign<CoAgent>(newAgent);
      registry.assign<CoKinematics>(newAgent, 0.5, glm::vec2(0.0f, 0.0f), 0.0f);
      registry.assign<CoRespawnsOoB>(newAgent);
      registry.assign<CoTarget>(newAgent);
    }

    static auto dist = std::uniform_real_distribution<float>(0.0f, 1.0f);
    static std::default_random_engine gen(23);

    for (int i = 0; i < 1000; i++) {
      auto newDebris = registry.create();
      registry.assign<CoItem>(newDebris, glm::vec2{0.0003, 0.0003} * 3.0f,
                              glm::vec2(dist(gen), dist(gen)), 1.2f);

      registry.assign<entt::tag<"tex_debris"_hs>>(newDebris);

      registry.assign<CoRespawnsOoB>(newDebris);
      registry.assign<CoAnimated>(newDebris, static_cast<float>(i % 2));
      registry.assign<CoKinematics>(newDebris, 0.5, glm::vec2(0.0f, 0.0f),
                                    0.0f);
    }
    // sim.step(0.003);
  }

  void loop();
  void draw();
  void handleKey(SDL_KeyboardEvent event);

  void processTorpedos();
  void newExplosion(glm::vec2 pos, float explosionDiam, entt::entity player,
                    int fragmentLevel = 0);
  void newExplosion(float explosionDiam, entt::entity player);
  void processExplosions();

  void shiftMap();

  glm::vec2 &pos(entt::entity entity) {
    return registry.get<CoItem>(entity).pos;
  }

  float &rot(entt::entity entity) {
    return registry.get<CoItem>(entity).rotation;
  }

  glm::vec2 &size(entt::entity entity) {
    return registry.get<CoItem>(entity).size;
  }

  glm::vec2 &force(entt::entity entity) {
    return registry.get<CoKinematics>(entity).force;
  }

  int &bumpCount(entt::entity entity) {
    return registry.get<CoKinematics>(entity).bumpCount;
  }

  float &age(entt::entity entity) {
    return registry.get<CoExplosion>(entity).age;
  }

  float &torpedoCooldown(entt::entity entity) {
    return registry.get<CoPlayer>(entity).torpedoCooldown;
  }
  float &torpedosLoaded(entt::entity entity) {
    return registry.get<CoPlayer>(entity).torpedosLoaded;
  }
  int &torpedosFired(entt::entity entity) {
    return registry.get<CoPlayer>(entity).torpedosFired;
  }

  float &frame(entt::entity entity) {
    return registry.get<CoAnimated>(entity).frame;
  }

  float &explosionDiam(entt::entity entity) {
    return registry.get<CoExplosion>(entity).explosionDiam;
  }

  int &fragmentLevel(entt::entity entity) {
    return registry.get<CoExplosion>(entity).fragmentLevel;
  }

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
  bool cheatMode = false;

  entt::registry registry;
  entt::registry &reg = registry;
  std::map<SDL_Keycode, bool> keysPressed;
  std::map<entt::component, Texture> textures;
};
