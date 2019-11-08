#include "ubootgl_app.hpp"
#include "controls.hpp"
#include "dtime.hpp"

#include "explosion.hpp"
#include "gl_error.hpp"
#include "torpedo.hpp"
#include <GL/glew.h>

#include <glm/gtx/transform.hpp>
#include <glm/vec2.hpp>
#include <vector>

using namespace std;

void UbootGlApp::loop() {

  simulationSteps = 1; // min(100.0, max(1.0, 0.02 / lastSimulationTime));

  float timestep = 0.1f / max(1.0, smoothedFrameRate * simulationSteps);

  double updateTime = dtime();
  double timeDelta = (updateTime - lastKeyUpdate) / simulationSteps;

  double sim_t1 = dtime();
  for (int simStep = 0; simStep < simulationSteps; simStep++) {

    registry.view<CoPlayer, CoItem, CoKinematics>().each([&](const auto pEnt,
                                                             const auto player,
                                                             const auto item,
                                                             const auto kin) {
      if (keysPressed[key_map[{player.keySet, CONTROLS::TURN_CLOCKWISE}]])
        rot(pEnt) -= 4.0 * timeDelta;
      if (keysPressed[key_map[{player.keySet,
                               CONTROLS::TURN_COUNTERCLOCKWISE}]])
        rot(pEnt) += 4.0 * timeDelta;

      if (keysPressed[key_map[{player.keySet, CONTROLS::THRUST_FORWARD}]])
        force(pEnt) += 8.0f * glm::vec2(cos(item.rotation), sin(item.rotation));
      if (keysPressed[key_map[{player.keySet, CONTROLS::THRUST_BACKWARD}]])
        force(pEnt) -= 3.0f * glm::vec2(cos(item.rotation), sin(item.rotation));
      if (keysPressed[key_map[{player.keySet, CONTROLS::LAUNCH_TORPEDO}]]) {
        if (player.torpedoCooldown < 0.0001 && player.torpedosLoaded > 1.0) {
          auto newTorpedo = registry.create();
          registry.assign<CoTorpedo>(newTorpedo);
          registry.assign<CoItem>(newTorpedo, glm::vec2{0.002, 0.0004},
                                  item.pos, item.rotation);
          registry.assign<CoKinematics>(
              newTorpedo, 0.15,
              kin.vel +
                  glm::vec2{cos(item.rotation), sin(item.rotation)} * 0.8f,
              kin.angVel);
          registry.assign<entt::tag<"tex_torpedo"_hs>>(newTorpedo);
          registry.assign<CoDeletedOoB>(newTorpedo);
          registry.assign<CoPlayerAligned>(newTorpedo, pEnt);
          registry.assign<CoTarget>(newTorpedo);
          torpedoCooldown(pEnt) = 0.014;
          torpedosLoaded(pEnt) -= 1.0;
          torpedosFired(pEnt)++;
        }
      }
      torpedoCooldown(pEnt) = max(0.0f, torpedoCooldown(pEnt) - timestep);
      torpedosLoaded(pEnt) =
          min(10.0f, torpedosLoaded(pEnt) + 1400.0f * timestep);
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
        registry.assign<CoItem>(newPlayer, glm::vec2{0.008, 0.002},
                                glm::vec2(0.5, 0.5), 0.0f);
        registry.assign<CoKinematics>(newPlayer, 1.3, glm::vec2(0, 0), 0.0f);
        registry.assign<entt::tag<"tex_ship"_hs>>(newPlayer);
        registry.assign<CoPlayer>(newPlayer, p);
        registry.assign<CoRespawnsOoB>(newPlayer);
        registry.assign<CoTarget>(newPlayer);
        registry.assign<CoPlayerAligned>(newPlayer, newPlayer);
      }
    }

    lastKeyUpdate = updateTime;

    simTime = 0;

    sim.step(timestep);
    sim.advectFloatingItems(registry);

    simTime += sim.dt;

    registry.view<CoRespawnsOoB, CoItem, CoKinematics>().less([&](auto &item,
                                                                  auto &kin) {
      static auto disx = std::uniform_real_distribution<float>(0.0f, sim.width);
      static auto disy =
          std::uniform_real_distribution<float>(0.0f, sim.height);
      static std::default_random_engine gen(std::random_device{}());

      glm::vec2 gridPos = item.pos / sim.h + 0.5f;
      while (gridPos.x > sim.width - 1 || gridPos.x < 1.0 ||
             gridPos.y > sim.height - 1 || gridPos.y < 1.0 ||
             sim.psampleFlagLinear(item.pos) < 0.01) {
        gridPos = glm::vec2(disx(gen), disy(gen));
        item.pos = gridPos * sim.h;
        kin.vel = {0.0f, 0.0f};
        kin.angVel = 0;
      }
    });

    registry.view<CoDeletedOoB, CoItem>().less([&](auto entity, auto &item) {
      glm::vec2 gridPos = item.pos / sim.h + 0.5f;
      if (gridPos.x > sim.width - 1 || gridPos.x < 1.0 ||
          gridPos.y > sim.height - 1 || gridPos.y < 1.0 ||
          sim.psampleFlagLinear(item.pos) < 0.01)
        registry.destroy(entity);
    });

    processTorpedos();
    processExplosions();

    registry.view<CoPlayer, CoItem, CoKinematics>().each(
        [&](auto pEnt, auto &player, auto &item, auto &kin) {
          if (player.deathtimer > 0.0f) {
            kin.vel = glm::vec2(0.0f, 0.0f);
            player.deathtimer -= simTime;
            if (player.deathtimer < 0.0f) {
              registry.assign<entt::tag<"tex_ship"_hs>>(pEnt);
              item.pos = glm::vec2(-1.0f, -1.0f);
              player.deathtimer = 0.0f;
            }
          }
        });

    registry.view<CoDecays>().each([&](auto entity, auto &decay) {
      static auto dist = std::uniform_real_distribution<float>(0.0f, 1.0f);
      static std::default_random_engine gen(std::random_device{}());
      if (pow(2.0f, -simTime / decay.halflife) < dist(gen))
        registry.destroy(entity);
    });

    classicSwarmAI(registry, sim.flag, sim.h);
  }
  double sim_t2 = dtime();
  lastSimulationTime = (sim_t2 - sim_t1) / simulationSteps;
  double thisFrameTime = dtime();
  smoothedFrameRate =
      0.95 * smoothedFrameRate + 0.05 / (thisFrameTime - lastFrameTime);

  frameTimes.add(1000.0f * (thisFrameTime - lastFrameTime));

  lastFrameTime = thisFrameTime;
}

void UbootGlApp::handleKey(SDL_KeyboardEvent event) {

  switch (event.keysym.sym) {
  case SDLK_ESCAPE:
    exit(0);
  default:
    keysPressed[event.keysym.sym] = event.state;
  }
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

  DrawTracers::updateTracers(sim.getVX(), sim.getVY(), sim.getFlag(), sim.width,
                             sim.height, simTime, sim.pwidth);

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

    Draw2DBuf::draw_mag(sim.getVX(), sim.getVY(), sim.width, sim.height, PVM,
                        sim.pwidth);
    Draw2DBuf::draw_flag(rock_texture, sim.getFlag(), sim.width, sim.height,
                         PVM, sim.pwidth);

    DrawTracers::draw(sim.width, sim.height, PVM, sim.pwidth);

    DrawFloatingItems::draw(
        registry, registry.type<entt::tag<"tex_debris1"_hs>>(),
        textures[registry.type<entt::tag<"tex_debris1"_hs>>()], PVM, 1.0f,
        true);
    DrawFloatingItems::draw(
        registry, registry.type<entt::tag<"tex_debris2"_hs>>(),
        textures[registry.type<entt::tag<"tex_debris2"_hs>>()], PVM, 1.0f,
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
  Draw2DBuf::draw_flag(black_texture, sim.getFlag(), sim.width, sim.height, PVM,
                       sim.pwidth);

  DrawFloatingItems::draw(
      registry, registry.type<entt::tag<"tex_explosion"_hs>>(),
      textures[registry.type<entt::tag<"tex_explosion"_hs>>()], PVM, 2.0f,
      true);
  DrawFloatingItems::draw(registry, registry.type<entt::tag<"tex_ship"_hs>>(),
                          textures[registry.type<entt::tag<"tex_ship"_hs>>()],
                          PVM, 4.0f, false);
  DrawFloatingItems::draw(
      registry, registry.type<entt::tag<"tex_torpedo"_hs>>(),
      textures[registry.type<entt::tag<"tex_torpedo"_hs>>()], PVM, 3.0f, false);

  ImGui::SetNextWindowPos(ImVec2(200, 10));
  ImGui::SetNextWindowSize(ImVec2(300, 300));
  ImGui::Begin("SideBar", &p_open,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

  ImGui::TextWrapped("Total: %.1f, %.1f, %.1f ", frameTimes.avg(),
                     frameTimes.high1pct(), frameTimes.largest());
  ImGui::TextWrapped("GFX: %.1f, %.1f, %.1f ", gfxTimes.avg(),
                     gfxTimes.high1pct(), gfxTimes.largest());

  ImGui::PlotLines("", frameTimes.data().data(), frameTimes.data().size(), 0,
                   NULL, 0, frameTimes.largest(), ImVec2(300, 80));
  ImGui::PlotLines("", gfxTimes.data().data(), gfxTimes.data().size(), 0, NULL,
                   0, gfxTimes.largest(), ImVec2(300, 80));

  ImGui::End();

  double graphicsT2 = dtime();
  gfxTimes.add((graphicsT2 - graphicsT1) * 1000.0);
}
