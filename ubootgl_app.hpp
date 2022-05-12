#pragma once
#include "components.hpp"
#include "draw_2dbuf.hpp"
#include "draw_floating_items.hpp"
#include "draw_tracers_cs.hpp"
#include "entt/core/hashed_string.hpp"
#include "entt/entity/helper.hpp"
#include "entt/entity/registry.hpp"

#include "frame_times.hpp"
#include "imgui/imgui.h"
#include "sdl_gl.hpp"
#include "simulation.hpp"
#include "swarm.hpp"
#include "terrain_generator.hpp"
#include "texture.hpp"
#include "velocity_textures.hpp"
#include <glm/vec2.hpp>
#include <iostream>
#include <map>
#include <random>
#include <vector>
#include <atomic>
using namespace entt::literals;

class UbootGlApp {
public:
  UbootGlApp()
      : sim("resources/level2_hires.png", 0.8, 0.001f),
        rock_texture("resources/rock_texture2.png"), joyAxis(4),
        joyButtonPressed(4) {
    Draw2DBuf::init();
    DrawTracersCS::init();
    DrawFloatingItems::init();
    VelocityTextures::init(sim.width, sim.height);

    scale = 3.0;

    textures.emplace(entt::type_hash<entt::tag<"tex_ship"_hs>>::value(),
                     Texture("resources/ship2.png", 1, 4));
    textures.emplace(entt::type_hash<entt::tag<"tex_debris1"_hs>>::value(),
                     Texture("resources/debris1.png"));
    textures.emplace(entt::type_hash<entt::tag<"tex_debris2"_hs>>::value(),
                     Texture("resources/debris2.png"));
    textures.emplace(entt::type_hash<entt::tag<"tex_debris"_hs>>::value(),
                     Texture("resources/debris.png", 2, 1));
    textures.emplace(entt::type_hash<entt::tag<"tex_agent"_hs>>::value(),
                     Texture("resources/agent2.png"));
    textures.emplace(entt::type_hash<entt::tag<"tex_torpedo"_hs>>::value(),
                     Texture("resources/torpedo.png"));
    textures.emplace(entt::type_hash<entt::tag<"tex_explosion"_hs>>::value(),
                     Texture("resources/explosion_fullalpha.png", 4, 4));

    for (int i = 0; i < 6; i++) {
      auto newAgent = registry.create();
      registry.emplace<CoItem>(newAgent, glm::vec2{0.005f, 0.002f},
                               glm::vec2(-1, -1), 0.0f);
      registry.emplace<entt::tag<"tex_agent"_hs>>(newAgent);
      registry.emplace<CoAgent>(newAgent);
      registry.emplace<CoKinematics>(newAgent, 0.5, glm::vec2(0.0f, 0.0f),
                                     0.0f);
      registry.emplace<CoRespawnsOoB>(newAgent);
      registry.emplace<CoTarget>(newAgent);
    }

    static auto dist = std::uniform_real_distribution<float>(0.0f, 1.0f);
    static std::default_random_engine gen(23);

    for (int i = 0; i < 10; i++) {
      auto newDebris = registry.create();
      registry.emplace<CoItem>(newDebris, glm::vec2{0.0003, 0.0003} * 3.0f,
                               glm::vec2(dist(gen), dist(gen)), 1.2f);

      registry.emplace<entt::tag<"tex_debris"_hs>>(newDebris);

      registry.emplace<CoRespawnsOoB>(newDebris);
      registry.emplace<CoAnimated>(newDebris, static_cast<float>(i % 2));
      registry.emplace<CoKinematicsSimple>(newDebris, 0.5, glm::vec2(0.0f, 0.0f),
                                     0.0f);
    }
    TerrainGenerator::init(sim.flag);
  }

  void loop();
  void draw();
  void sim_loop(void);
  void handleKey(SDL_KeyboardEvent event);
  void handleJoyAxis(SDL_JoyAxisEvent event);
  void handleJoyButton(SDL_JoyButtonEvent event);
  void processTorpedos();
  void newExplosion(glm::vec2 pos, float explosionDiam, entt::entity player,
                    int fragmentLevel = 0);
  void newExplosion(float explosionDiam, entt::entity player);
  void processExplosions();
  void launchTorpedo(entt::entity player);

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
  FrameTimes simTimes;
  FrameTimes gameLogicTimes;
  double lastSimulationTime = 1.0;
  float scale;
  float gameTimeStep = 0;
  float simTimeStep = 0;
  std::atomic<int> mapShifts = 0;
  std::mutex mapShiftMutex;
  SdlGl vis;
  Simulation sim;
  Texture rock_texture;
  Texture black_texture = Texture("resources/black.png");
  float texture_offset = 0.0f;

  double lastKeyUpdate;
  bool gameRunning = true ;
  bool cheatMode = false;


  entt::registry registry;
  entt::registry &reg = registry;
  std::map<SDL_Keycode, bool> keysPressed;
  std::vector<std::map<int, int>> joyAxis;
  std::vector<std::map<int, bool>> joyButtonPressed;
  std::map<entt::id_type, Texture> textures;
};
