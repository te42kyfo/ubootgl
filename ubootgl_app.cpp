#include "ubootgl_app.hpp"
#include "controls.hpp"
#include "dtime.hpp"

#include "explosion.hpp"
#include "gl_error.hpp"
#include "torpedo.hpp"
#include "velocity_textures.hpp"
#include <GL/glew.h>

#include <glm/gtx/transform.hpp>
#include <glm/vec2.hpp>
#include <omp.h>
#include <vector>

using namespace std;

void UbootGlApp::loop() {

  gameTimeStep = 0.1f / max(5.0, smoothedFrameRate);

  double updateTime = dtime();
  double timeDelta = (updateTime - lastKeyUpdate);

  double gameLogicT1 = dtime();

  registry.view<CoPlayer, CoItem, CoKinematics>().each([&](const auto pEnt,
                                                           const auto player,
                                                           const auto item,
                                                           const auto kin) {
    float joyAngle = fmod(
        atan2(-joyAxis[player.keySet][1], joyAxis[player.keySet][0]) + 2 * M_PI,
        2 * M_PI);

    float diffAngle = (joyAngle - item.rotation);
    if (abs(diffAngle) > M_PI)
      diffAngle -= glm::sign(diffAngle) * M_PI * 2;

    float dir = glm::sign(diffAngle);

    auto joyVec =
        glm::vec2(joyAxis[player.keySet][0] + 0.001, joyAxis[player.keySet][1]);
    rot(pEnt) += timeDelta * dir * 0.00007 * length(joyVec) *
                 (0.2 + min((float)M_PI * 0.5f, abs(diffAngle)));

    if (keysPressed[key_map[{player.keySet, CONTROLS::TURN_CLOCKWISE}]])
      rot(pEnt) -= 4.0 * timeDelta;
    if (keysPressed[key_map[{player.keySet, CONTROLS::TURN_COUNTERCLOCKWISE}]])
      rot(pEnt) += 4.0 * timeDelta;

    registry.get<CoAnimated>(pEnt).frame = 0.0;
    if (keysPressed[key_map[{player.keySet, CONTROLS::THRUST_FORWARD}]] ||
        joyButtonPressed[player.keySet][4]) {
      force(pEnt) += 12.0f * glm::vec2(cos(item.rotation), sin(item.rotation));
      registry.get<CoAnimated>(pEnt).frame = 1.0;
    }
    if (keysPressed[key_map[{player.keySet, CONTROLS::THRUST_BACKWARD}]]) {
      force(pEnt) -= 3.0f * glm::vec2(cos(item.rotation), sin(item.rotation));
      registry.get<CoAnimated>(pEnt).frame = 1.0;
    }
    if (keysPressed[key_map[{player.keySet, CONTROLS::LAUNCH_TORPEDO}]] ||
        joyButtonPressed[player.keySet][5]) {
      if (player.torpedoCooldown < 0.0001 && player.torpedosLoaded > 1.0) {
        auto newTorpedo = registry.create();
        registry.assign<CoTorpedo>(newTorpedo);
        registry.assign<CoItem>(newTorpedo, glm::vec2{0.003, 0.0006}, item.pos,
                                item.rotation);
        registry.assign<CoKinematics>(
            newTorpedo, 0.15,
            kin.vel + glm::vec2{cos(item.rotation), sin(item.rotation)} * 0.8f,
            kin.angVel);
        registry.assign<entt::tag<"tex_torpedo"_hs>>(newTorpedo);
        registry.assign<CoDeletedOoB>(newTorpedo);
        registry.assign<CoPlayerAligned>(newTorpedo, pEnt);
        registry.assign<CoTarget>(newTorpedo);
        torpedoCooldown(pEnt) = cheatMode ? 0.005 : 0.02;
        torpedosLoaded(pEnt) -= cheatMode ? 0.0 : 1.0;
        torpedosFired(pEnt)++;
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
      registry.assign<CoItem>(newPlayer, glm::vec2{0.008, 0.0024},
                              glm::vec2(0.2, 0.2), 0.0f);
      registry.assign<CoKinematics>(newPlayer, 1.3, glm::vec2(0, 0), 0.0f);
      registry.assign<entt::tag<"tex_ship"_hs>>(newPlayer);
      registry.assign<CoPlayer>(newPlayer, p);
      registry.assign<CoRespawnsOoB>(newPlayer);
      registry.assign<CoTarget>(newPlayer);
      registry.assign<CoAnimated>(newPlayer, 0.0f);
      registry.assign<CoPlayerAligned>(newPlayer, newPlayer);
    }
  }

  lastKeyUpdate = updateTime;

  sim.advectFloatingItems(registry, gameTimeStep);

  processTorpedos();
  processExplosions();

  registry.view<CoRespawnsOoB, CoItem, CoKinematics>().less([&](auto &item,
                                                                auto &kin) {
    static auto disx = std::uniform_real_distribution<float>(0.0f, sim.width);
    static auto disy = std::uniform_real_distribution<float>(0.0f, sim.height);
    static std::default_random_engine gen(std::random_device{}());

    glm::vec2 gridPos = item.pos / sim.h + 0.5f;
    while (gridPos.x > sim.width - 1 || gridPos.x < 1.0 ||
           gridPos.y > sim.height - 1 || gridPos.y < 1.0 ||
           sim.psampleFlagLinear(item.pos) < 0.01) {

      gridPos = glm::vec2(disx(gen), disy(gen));
      item.pos = gridPos * sim.h;
      kin.vel = {0.0f, 0.0f};
      kin.angVel = 0;
      kin.force = {0.0f, 0.0f};
    }
  });

  registry.view<CoDeletedOoB, CoItem>().less([&](auto entity, auto &item) {
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
            registry.assign<entt::tag<"tex_ship"_hs>>(pEnt);
            registry.assign<CoTarget>(pEnt);
            item.pos = glm::vec2(-1.0f, -1.0f);
            player.state = PLAYER_STATE::ALIVE_PROTECTED;
            player.timer = 0.4f;
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

  classicSwarmAI(registry, sim.flag, sim.getVX(), sim.getVY(), sim.vx.width,
                 sim.h);

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
    exit(0);
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

  for (int y = 0; y < sim.vx.height; y++) {
    for (int x = 2; x < sim.vx.width; x++) {
      sim.vx(x - 1, y) = sim.vx(x, y);
    }
  }

  for (int y = 0; y < sim.vy.height; y++) {
    for (int x = 2; x < sim.vy.width; x++) {
      sim.vy(x - 1, y) = sim.vy(x, y);
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
  }
  for (int i = 0; i < 40; i++) {
    int y = rand() % (sim.flag.height - 2) + 1;
    if (fabs(sim.vx(sim.vx.width - 1, y)) < 0.01)
      sim.setGrids(glm::vec2(sim.flag.width - 1, y), rand() % 20 <= 7);
  }

  for (int y = 1; y < sim.flag.height - 2; y++) {
    float v =
        sim.flag(sim.flag.width - 2, y) - sim.flag(sim.flag.width - 2, y + 1);
    float v2 =
        sim.flag(sim.flag.width - 2, y - 1) - sim.flag(sim.flag.width - 2, y);

    if (rand() % 2 == 0) {
      if (v > 0 && rand() % 1 == 0) {
        sim.setGrids(glm::vec2(sim.flag.width - 1, y), 0.0);
      }
      if (v < 0 && rand() % 1 == 0) {
        sim.setGrids(glm::vec2(sim.flag.width - 1, y), 1.0);
      }
    } else {

      if (v2 > 0 && rand() % 1 == 0) {
        sim.setGrids(glm::vec2(sim.flag.width - 1, y), 1.0);
      }
      if (v2 < 0 && rand() % 1 == 0) {
        sim.setGrids(glm::vec2(sim.flag.width - 1, y), 0.0);
      }
    }
  }

  for (int y = 1; y < sim.flag.height - 2; y++) {
    float v = (sim.flag(sim.flag.width - 1, y - 1) +
               sim.flag(sim.flag.width - 1, y) * 1.0 +
               sim.flag(sim.flag.width - 1, y + 1)) /
              3.0f;
    if (v > 0.5f)
      sim.setGrids(glm::vec2(sim.flag.width - 1, y), 1.0);
    else if (v < 0.5f)
      sim.setGrids(glm::vec2(sim.flag.width - 1, y), 0.0);
  }

  float inletArea = 1.0f;
  for (int y = 0; y < sim.vy.height; y++) {
    inletArea += sim.flag(0, y);
  }
  for (int y = 0; y < sim.vy.height; y++) {
    sim.vx(0, y) = 30.0 / inletArea;
  }

  sim.mg.updateFields(sim.flag);

  registry.view<CoItem>().each(
      [&](auto &item) { item.pos.x -= sim.pwidth / (sim.flag.width - 1); });
}

void UbootGlApp::draw() {

  GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  int xsplits = 1;
  int ysplits = 1;
  if (registry.size<CoPlayer>() == 4) {
    xsplits = 2;
    ysplits = 2;
  } else {
    xsplits = max(1, (int)registry.size<CoPlayer>());
  }

  int displayWidth = ImGui::GetIO().DisplaySize.x;
  int displayHeight = ImGui::GetIO().DisplaySize.y;
  int renderWidth = displayWidth / xsplits * 0.99;
  int renderHeight = (displayHeight - 50) / ysplits;
  int renderOriginX = renderWidth;
  int renderOriginY = 0;

  bool p_open;

  double graphicsT1 = dtime();

  /*  for (int pid = 0; pid < players.size(); pid++) {
    DrawTracers::playerTracersAdd(
        pid,
        ((playerShips[pid].pos - glm::vec2{cos(playerShips[pid].rotation),
                                           sin(playerShips[pid].rotation)} *
                                     playerShips[pid].size.x * 0.2f)) /
            (sim.pwidth / (sim.width)));
            }*/

  VelocityTextures::updateFromStaggered(sim.vx_current.data(),
                                        sim.vy_current.data());
  DrawTracersCS::updateTracers(VelocityTextures::getVXYTex(),
                               VelocityTextures::getFlagTex(), sim.ivx.width,
                               sim.ivy.height, gameTimeStep, sim.pwidth);

  registry.view<CoPlayer, CoItem>().each([&](auto &player, auto &item) {
    renderOriginX = renderWidth * (player.keySet % xsplits * 1.01);
    renderOriginY = renderHeight * (player.keySet / xsplits) * 1.01;

    ImGui::SetNextWindowPos(
        ImVec2(renderOriginX + 2,
               displayHeight - (player.keySet / xsplits + 1) *
                                   ((displayHeight - 55) / ysplits)));

    ImGui::SetNextWindowSize(ImVec2(200, 100));
    ImGui::Begin(("Player" + to_string(player.keySet)).c_str(), &p_open,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    ImGui::Text("Player %d", player.keySet);
    ImGui::Separator();
    ImGui::Text("Kills: %d, Deaths: %d", player.kills, player.deaths);
    ImGui::Text("Torpedos: %.0f", player.torpedosLoaded);
    if (player.state == PLAYER_STATE::ALIVE_PROTECTED) {
      ImGui::Text("PROTECTED %.2f", player.timer);
    } else if (player.state == PLAYER_STATE::RESPAWNING) {
      ImGui::Text("RESPAWNING %.2f", player.timer);
    }
    ImGui::End();

    GL_CALL(
        glViewport(renderOriginX, renderOriginY, renderWidth, renderHeight));
    GL_CALL(glEnable(GL_FRAMEBUFFER_SRGB));

    GL_CALL(glEnable(GL_BLEND));
    GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE));

    // Projection-View-Matrix
    glm::mat4 PVM(1.0f);
    // Projection

    PVM = glm::scale(
        PVM, glm::vec3(2 * scale * xsplits,
                       2 * scale * xsplits * (float)renderWidth / renderHeight,
                       1.0f));
    // View
    // PVM = glm::rotate(PVM, playerShips[i].rotation - glm::half_pi<float>(),
    //                  glm::vec3(0.0f, 0.0f, -1.0f));
    PVM = glm::translate(PVM, glm::vec3(-item.pos, 0.0f));

    Draw2DBuf::draw_mag(VelocityTextures::getMagTex(), sim.ivx.width,
                        sim.ivx.height, PVM, sim.pwidth);
    Draw2DBuf::draw_flag(rock_texture, VelocityTextures::getFlagTex(),
                         sim.width, sim.height, PVM, sim.pwidth);

    DrawTracersCS::draw(PVM, renderWidth);

    DrawFloatingItems::draw(
        registry, registry.type<entt::tag<"tex_debris"_hs>>(),
        textures[registry.type<entt::tag<"tex_debris"_hs>>()], PVM, 1.0f, true);

    DrawFloatingItems::draw(
        registry, registry.type<entt::tag<"tex_debris1"_hs>>(),
        textures[registry.type<entt::tag<"tex_debris1"_hs>>()], PVM, 1.0f,
        true);
    DrawFloatingItems::draw(
        registry, registry.type<entt::tag<"tex_torpedo"_hs>>(),
        textures[registry.type<entt::tag<"tex_torpedo"_hs>>()], PVM, 1.0f,
        false);

    DrawFloatingItems::draw(
        registry, registry.type<entt::tag<"tex_agent"_hs>>(),
        textures[registry.type<entt::tag<"tex_agent"_hs>>()], PVM, 1.0f, false);

    DrawFloatingItems::draw(registry, registry.type<entt::tag<"tex_ship"_hs>>(),
                            textures[registry.type<entt::tag<"tex_ship"_hs>>()],
                            PVM, 1.0f);

    DrawFloatingItems::draw(
        registry, registry.type<entt::tag<"tex_explosion"_hs>>(),
        textures[registry.type<entt::tag<"tex_explosion"_hs>>()], PVM, 1.0f,
        true);
  });

  renderOriginX = displayWidth * 0.4;
  renderOriginY = displayHeight * 0.0;
  renderWidth = displayWidth * 0.2;
  renderHeight = displayWidth * 0.2;

  GL_CALL(glViewport(renderOriginX, renderOriginY, renderWidth, renderHeight));

  GL_CALL(glBlendFunc(GL_ONE, GL_ZERO));

  // Projection-View-Matrix
  glm::mat4 PVM(1.0f);
  PVM = glm::scale(
      PVM,
      glm::vec3(1.0f, 1.0f * (float)renderWidth / renderHeight, 1.0f) * 2.0f);
  PVM = glm::translate(PVM, glm::vec3(-0.5f, -0.5f, 0.0f));
  Draw2DBuf::draw_flag(black_texture, VelocityTextures::getFlagTex(), sim.width,
                       sim.height, PVM, sim.pwidth);

  DrawFloatingItems::draw(registry, registry.type<entt::tag<"tex_ship"_hs>>(),
                          textures[registry.type<entt::tag<"tex_ship"_hs>>()],
                          PVM, 5.0f, false);
  DrawFloatingItems::draw(registry, registry.type<entt::tag<"tex_agent"_hs>>(),
                          textures[registry.type<entt::tag<"tex_agent"_hs>>()],
                          PVM, 3.0f, false);
  DrawFloatingItems::draw(
      registry, registry.type<entt::tag<"tex_torpedo"_hs>>(),
      textures[registry.type<entt::tag<"tex_torpedo"_hs>>()], PVM, 3.0f, false);

  ImGui::SetNextWindowPos(ImVec2(200, 10));
  ImGui::SetNextWindowSize(ImVec2(300, 400));
  ImGui::Begin("SideBar", &p_open,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

  ImGui::TextWrapped("Total: %.1f, %.1f, %.1f ", frameTimes.avg(),
                     frameTimes.high1pct(), frameTimes.largest());
  ImGui::TextWrapped("GFX: %.1f, %.1f, %.1f ", gfxTimes.avg(),
                     gfxTimes.high1pct(), gfxTimes.largest());
  ImGui::TextWrapped("SIM: %.1f, %.1f, %.1f ", simTimes.avg(),
                     simTimes.high1pct(), simTimes.largest());

  ImGui::PlotLines("", frameTimes.data().data(), frameTimes.data().size(), 0,
                   NULL, 0, frameTimes.largest(), ImVec2(300, 80));
  ImGui::PlotLines("", gfxTimes.data().data(), gfxTimes.data().size(), 0, NULL,
                   0, gfxTimes.largest(), ImVec2(300, 80));
  ImGui::PlotLines("", simTimes.data().data(), simTimes.data().size(), 0, NULL,
                   0, simTimes.largest(), ImVec2(300, 80));

  ImGui::End();

  double graphicsT2 = dtime();
  gfxTimes.add((graphicsT2 - graphicsT1) * 1000.0);
}
