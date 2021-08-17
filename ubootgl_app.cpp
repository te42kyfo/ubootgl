#include "ubootgl_app.hpp"
#include "controls.hpp"
#include "dtime.hpp"

#include "velocity_textures.hpp"

#include <vector>

using namespace std;

void UbootGlApp::loop() {
  gameTimeStep = 0.1f / max(5.0, smoothedFrameRate);

  double updateTime = dtime();
  double timeDelta = (updateTime - lastKeyUpdate);

  double gameLogicT1 = dtime();

  registry.view<CoPlayer, CoItem>().each([&](const auto pEnt, const auto player,
                                             const auto item) {
    float joyAngle = fmod(
        atan2(-joyAxis[player.keySet][1], joyAxis[player.keySet][0]) + 2 * M_PI,
        2 * M_PI);

    float diffAngle = (joyAngle - item.rotation);
    if (abs(diffAngle) > M_PI)
      diffAngle -= glm::sign(diffAngle) * M_PI * 2;

    float dir = glm::sign(diffAngle);

    auto joyVec =
        glm::vec2(joyAxis[player.keySet][0] + 0.001, joyAxis[player.keySet][1]);
    registry.get<CoKinematics>(pEnt).angVel = 0.0f;
    registry.get<CoKinematics>(pEnt).angVel =
        dir * 0.001 * length(joyVec) *
        (0.3 + min((float)M_PI * 0.5f, abs(diffAngle)));

    if (keysPressed[key_map[{player.keySet, CONTROLS::TURN_CLOCKWISE}]]) {
      // rot(pEnt) -= 4.0 * timeDelta;
      registry.get<CoKinematics>(pEnt).angVel = -50.0;
    }
    if (keysPressed[key_map[{player.keySet,
                             CONTROLS::TURN_COUNTERCLOCKWISE}]]) {
      // rot(pEnt) += 4.0 * timeDelta;
      registry.get<CoKinematics>(pEnt).angVel = 50.0;
    }
    registry.get<CoAnimated>(pEnt).frame =
        (-cos(player.timer * 180.0) + 1) * 0.5f;
    if (keysPressed[key_map[{player.keySet, CONTROLS::THRUST_FORWARD}]] ||
        joyButtonPressed[player.keySet][4]) {
      force(pEnt) += 12.0f * glm::vec2(cos(item.rotation), sin(item.rotation));
      registry.get<CoAnimated>(pEnt).frame =
          (-cos(player.timer * 180.0) + 1) * 0.5f + 2.0f;
    }
    if (keysPressed[key_map[{player.keySet, CONTROLS::THRUST_BACKWARD}]]) {
      force(pEnt) -= 3.0f * glm::vec2(cos(item.rotation), sin(item.rotation));
      registry.get<CoAnimated>(pEnt).frame =
          (-cos(player.timer * 180.0) + 1) * 0.5f + 2.0f;
    }
    if (keysPressed[key_map[{player.keySet, CONTROLS::LAUNCH_TORPEDO}]] ||
        joyButtonPressed[player.keySet][5]) {
      if (player.torpedoCooldown < 0.0001 && player.torpedosLoaded > 1.0 &&
          player.state == PLAYER_STATE::ALIVE) {
        launchTorpedo(pEnt);
      }
    }
    torpedoCooldown(pEnt) = max(0.0f, torpedoCooldown(pEnt) - gameTimeStep);
    torpedosLoaded(pEnt) =
        min(8.0f, torpedosLoaded(pEnt) + 10.0f * gameTimeStep);
  });

  if (keysPressed[SDLK_PAGEUP])
    scale *= pow(2, timeDelta);
  if (keysPressed[SDLK_PAGEDOWN])
    scale *= pow(0.5, timeDelta);

  int playerCount = registry.size<CoPlayer>();
  if (keysPressed[SDLK_1])
    playerCount = 1;
  if (keysPressed[SDLK_2])
    playerCount = 2;
  if (keysPressed[SDLK_3])
    playerCount = 3;
  if (keysPressed[SDLK_4])
    playerCount = 4;
  playerCount = max(1, playerCount);

  if (playerCount != (int)registry.size<CoPlayer>()) {
    auto playerView = registry.view<CoPlayer>();
    registry.destroy(begin(playerView), end(playerView));

    for (int p = 0; p < playerCount; p++) {
      auto newPlayer = registry.create();
      registry.emplace<CoItem>(newPlayer, glm::vec2{0.009, 0.0022},
                               glm::vec2(0.2, 0.2), 0.0f);
      registry.emplace<CoKinematics>(newPlayer, 1.3, glm::vec2(0, 0), 0.0f);
      registry.emplace<entt::tag<"tex_ship"_hs>>(newPlayer);
      registry.emplace<CoPlayer>(newPlayer, p);
      registry.emplace<CoRespawnsOoB>(newPlayer);
      registry.emplace<CoTarget>(newPlayer);
      registry.emplace<CoAnimated>(newPlayer, 0.0f);
      registry.emplace<CoPlayerAligned>(newPlayer, newPlayer);
      registry.emplace<CoHasTracer>(newPlayer);
    }
  }

  lastKeyUpdate = updateTime;

  sim.advectFloatingItems(registry, gameTimeStep);

  processTorpedos();
  processExplosions();

  registry.view<CoRespawnsOoB, CoItem, CoKinematics>().each([&](auto entity,
                                                                auto &item,
                                                                auto &kin) {
    static auto disx = std::uniform_real_distribution<float>(0.0f, sim.width);
    static auto disy = std::uniform_real_distribution<float>(0.0f, sim.height);
    static std::default_random_engine gen(std::random_device{}());

    glm::vec2 gridPos = item.pos / sim.h + 0.5f;
    bool respawned = false;
    while (gridPos.x > sim.width - 1 || gridPos.x < 1.0 ||
           gridPos.y > sim.height - 1 || gridPos.y < 1.0 ||
           sim.psampleFlagLinear(item.pos) < 0.01) {

      respawned = true;
      gridPos = glm::vec2(disx(gen), disy(gen));
      item.pos = gridPos * sim.h;
      kin.vel = {0.0f, 0.0f};
      kin.angVel = 0;
      kin.force = {0.0f, 0.0f};
    }
    auto player = registry.try_get<CoPlayer>(entity);
    if (player && respawned) {
      player->state = PLAYER_STATE::ALIVE_PROTECTED;
      player->timer += 0.15;
    }
  });

  registry.view<CoDeletedOoB, CoItem>().each([&](auto entity, auto &item) {
    glm::vec2 gridPos = item.pos / sim.h + 0.5f;
    if (gridPos.x > sim.width - 1 || gridPos.x < 1.0 ||
        gridPos.y > sim.height - 1 || gridPos.y < 1.0 ||
        sim.psampleFlagLinear(item.pos) < 0.01)
      registry.destroy(entity);
  });

  registry.view<CoPlayer, CoItem, CoKinematics>().each(
      [&](auto pEnt, auto &player, auto &item, auto &kin) {
        player.timer = max(0.0f, player.timer - gameTimeStep);

        if (player.state == PLAYER_STATE::ALIVE)
          return;
        if (player.state == PLAYER_STATE::KILLED) {
          player.timer = 0.2f;
          player.deaths++;
          registry.remove<entt::tag<"tex_ship"_hs>>(pEnt);
          registry.remove<CoTarget>(pEnt);
          player.state = PLAYER_STATE::RESPAWNING;
          return;
        }
        if (player.state == PLAYER_STATE::RESPAWNING) {
          kin.vel = glm::vec2(0.0f, 0.0f);
          if (player.timer <= 0.0f) {
            registry.emplace<entt::tag<"tex_ship"_hs>>(pEnt);
            registry.emplace<CoTarget>(pEnt);
            item.pos = glm::vec2(-1.0f, -1.0f);
            player.state = PLAYER_STATE::ALIVE_PROTECTED;
            player.timer = 0.3f;
          }
          return;
        }
        if (player.state == PLAYER_STATE::ALIVE_PROTECTED) {
          if (player.timer <= 0.0f) {
            player.state = PLAYER_STATE::ALIVE;
          }
        }
      });

  registry.view<CoDecays>().each([&](auto entity, auto &decay) {
    static auto dist = std::uniform_real_distribution<float>(0.0f, 1.0f);
    static std::default_random_engine gen(std::random_device{}());
    if (pow(2.0f, -gameTimeStep / decay.halflife) < dist(gen))
      registry.destroy(entity);
  });

  classicSwarmAI(registry, sim.flag, sim.vx, sim.vy, sim.h);

  scoped_lock lock(mapShiftMutex);
  {
    sim.mg.updateFields(sim.flag);
    VelocityTextures::uploadFlag(sim.getFlag());
    VelocityTextures::updateFromStaggered(sim.vx_current.data(),
                                          sim.vy_current.data());
    while (mapShifts > 0) {
      float shift = sim.pwidth / (sim.flag.width-1);
      registry.view<CoItem>().each(
          [&](auto &item) { item.pos.x -= shift; });
      texture_offset -=
          (float)rock_texture.width * 10.0f / sim.flag.width / sim.flag.width;
      DrawTracersCS::shiftPlayerTracers(-shift);
      DrawTracersCS::shiftFluidTracers(-shift);
      mapShifts--;
    }
  }

  double gameLogicT2 = dtime();

  gameLogicTimes.add(gameLogicT2 - gameLogicT1);

  double thisFrameTime = dtime();
  smoothedFrameRate =
      0.95 * smoothedFrameRate + 0.05 / (thisFrameTime - lastFrameTime);

  lastFrameTime = thisFrameTime;
}

void UbootGlApp::handleKey(SDL_KeyboardEvent event) {

  switch (event.keysym.sym) {
  case SDLK_ESCAPE:
    gameRunning = false;
    break;
  case SDLK_HASH:
    if (event.state)
      cheatMode ^= true;
    break;
  default:
    keysPressed[event.keysym.sym] = event.state;
  }
}

void UbootGlApp::handleJoyAxis(SDL_JoyAxisEvent event) {
  joyAxis[event.which][event.axis] = event.value;
}

void UbootGlApp::handleJoyButton(SDL_JoyButtonEvent event) {
  joyButtonPressed[event.which][event.button] = event.state;
}
void UbootGlApp::shiftMap() {

  auto newLine = TerrainGenerator::generateLine(sim.flag);

  {
    scoped_lock lock(mapShiftMutex);
    for (int y = 0; y < sim.vx.height; y++) {
      for (int x = 2; x < sim.vx.width; x++) {
        sim.vx(x - 1, y) = sim.vx(x, y);
        sim.vx.b(x - 1, y) = sim.vx(x, y);
      }
    }

    for (int y = 0; y < sim.vy.height; y++) {
      for (int x = 2; x < sim.vy.width; x++) {
        sim.vy(x - 1, y) = sim.vy(x, y);
        sim.vy.b(x - 1, y) = sim.vy(x, y);
      }
    }

    for (int y = 0; y < sim.p.height; y++) {
      for (int x = 1; x < sim.p.width; x++) {
        sim.p(x - 1, y) = sim.p(x, y);
      }
    }

    for (int y = 0; y < sim.flag.height; y++) {
      for (int x = 1; x < sim.flag.width; x++) {
        sim.setGrids(glm::vec2(x - 1, y), sim.flag(x, y));
      }
      sim.setGrids(glm::vec2(sim.flag.width - 1, y),
                   sim.flag(sim.flag.width - 2, y));
    }

    for (int y = 0; y < sim.flag.height; y++) {
      sim.setGrids(glm::vec2(sim.flag.width - 1, y), newLine[y]);
    }

    float inletArea = 1.0f;
    for (int y = 0; y < sim.vy.height; y++) {
      inletArea += sim.flag(0, y);
    }
    for (int y = 0; y < sim.vx.height; y++) {
      sim.vx.f(0, y) = 20.0f / inletArea;
      sim.vx.b(0, y) = 20.0f / inletArea;
    }
    sim.saveCurrentVelocityFields();
  }
  sim.mg.updateFields(sim.flag);

  mapShifts++;
}
