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

template <typename T>
void respawnOutOfBounds(T begin, T end, int width, int height, Simulation &sim,
                        float h) {

  static auto disx = std::uniform_real_distribution<float>(0.0f, width);
  static auto disy = std::uniform_real_distribution<float>(0.0f, height);
  static std::default_random_engine gen(std::random_device{}());

  for (auto it = begin; it != end; it++) {
    glm::vec2 gridPos = it->pos / h + 0.5f;
    while (gridPos.x >= width - 2 || gridPos.x <= 1.0 ||
           gridPos.y >= height - 2 || gridPos.y <= 1.0 ||
           sim.psampleFlagLinear(it->pos) < 0.01) {
      gridPos = glm::vec2(disx(gen), disy(gen));
      it->pos = gridPos * h;
      it->vel = {0.0f, 0.0f};
      it->angVel = 0;
    }
  }
}
template <typename T>
auto removeOutOfBounds(T begin, T end, int width, int height, Simulation &sim,
                       float h) {

  return remove_if(begin, end, [&](auto &it) {
    glm::vec2 gridPos = it.pos / h + 0.5f;
    return (gridPos.x >= width - 2 || gridPos.x <= 1.0 ||
            gridPos.y >= height - 2 || gridPos.y <= 1.0 ||
            sim.psampleFlagLinear(it.pos) < 0.01);
  });
}

void UbootGlApp::loop() {

  simulationSteps = min(100.0, max(1.0, 0.02 / lastSimulationTime));

  float timestep = 0.1f / max(1.0, smoothedFrameRate * simulationSteps);

  double updateTime = dtime();
  double timeDelta = (updateTime - lastKeyUpdate) / simulationSteps;

  double sim_t1 = dtime();
  for (int simStep = 0; simStep < simulationSteps; simStep++) {

    for (unsigned int player = 0; player < playerShips.size(); player++) {
      auto &ship = playerShips[player];
      if (keysPressed[key_map[{player, CONTROLS::TURN_CLOCKWISE}]])
        ship.rotation -= 4.0 * timeDelta;
      if (keysPressed[key_map[{player, CONTROLS::TURN_COUNTERCLOCKWISE}]])
        ship.rotation += 4.0 * timeDelta;
      if (keysPressed[key_map[{player, CONTROLS::THRUST_FORWARD}]])
        ship.force += 8.0f * glm::vec2(cos(ship.rotation), sin(ship.rotation));
      if (keysPressed[key_map[{player, CONTROLS::THRUST_BACKWARD}]])
        ship.force -= 3.0f * glm::vec2(cos(ship.rotation), sin(ship.rotation));
      if (keysPressed[key_map[{player, CONTROLS::LAUNCH_TORPEDO}]]) {
        if (players[player].torpedoCooldown < 0.0001 &&
            players[player].torpedosLoaded > 1.0) {
          torpedos.push_back(
              {glm::vec2{0.002, 0.0004}, 0.15,
               ship.vel +
                   glm::vec2{cos(ship.rotation), sin(ship.rotation)} * 0.2f,
               ship.pos, ship.rotation, ship.angVel, &(textures[4]),
               (int)player});
          players[player].torpedoCooldown = 0.015;
          players[player].torpedosLoaded -= 1.0;
          players[player].torpedosFired++;
        }
      }
      players[player].torpedoCooldown =
          max(0.0f, players[player].torpedoCooldown - timestep);
      players[player].torpedosLoaded =
          min(8.0f, players[player].torpedosLoaded + 10.0f * timestep);
    }

    if (keysPressed[SDLK_PAGEUP])
      scale *= pow(2, timeDelta);
    if (keysPressed[SDLK_PAGEDOWN])
      scale *= pow(0.5, timeDelta);

    if (keysPressed[SDLK_1])
      playerCount = 1;
    if (keysPressed[SDLK_2])
      playerCount = 2;
    if (keysPressed[SDLK_3])
      playerCount = 3;
    if (keysPressed[SDLK_4])
      playerCount = 4;

    if (playerCount != playerShips.size()) {
      players.clear();
      playerShips.clear();
      for (unsigned int p = 0; p < playerCount; p++) {
        playerShips.push_back({glm::vec2{0.006, 0.0015}, 1.3, glm::vec2(0, 0),
                               glm::vec2(-0.5, -0.5), 0.0, 0.0, &textures[0],
                               (int)p});
        players.push_back(Player());
      }
    }

    lastKeyUpdate = updateTime;

    simTime = 0;

    sim.step(timestep);

    sim.advectFloatingItems(&*begin(playerShips), &*end(playerShips));
    sim.advectFloatingItems(&*begin(debris), &*end(debris));
    sim.advectFloatingItems(&*begin(swarm.agents), &*end(swarm.agents));
    sim.advectFloatingItems(&*begin(torpedos), &*end(torpedos));

    simTime += sim.dt;

    respawnOutOfBounds(&*begin(swarm.agents), &*end(swarm.agents), sim.width,
                       sim.height, sim, sim.h);
    respawnOutOfBounds(&*begin(playerShips), &*end(playerShips), sim.width,
                       sim.height, sim, sim.h);

    // sim.sinks.clear();

    float explosionDiam = 0.005;
    torpedos.erase(
        remove_if(
            begin(torpedos), end(torpedos),
            [=](auto &t) {
              bool explode = processTorpedo(
                  t, &*begin(swarm.agents), &*end(swarm.agents),
                  &*begin(playerShips), &*end(playerShips), simTime);
              if (explode) {
                explosions.push_back({glm::vec2{0.01, 0.01}, 0.01, t.vel, t.pos,
                                      rand() % 100 * 100.0f * 2.0f * 3.14f, 0,
                                      &(textures[5]), t.player});
                explosions.back().age = 0.02f;

                sim.sinks.push_back(glm::vec3(t.pos, 100.0f));
                float diam = explosionDiam / sim.h;
                for (int y = -diam; y <= diam; y++) {
                  for (int x = -diam; x <= diam; x++) {
                    auto gridC =
                        (t.pos + glm::vec2(x, y) * sim.h) / sim.h + 0.5f;
                    if (x * x + y * y > diam * diam)
                      continue;
                    if (sim.flag(gridC) < 1.0) {
                      for (int i = 0; i < 50; i++) {
                        float velangle =
                            orientedAngle(t.vel, glm::vec2{0.0f, 1.0f}) +
                            (rand() % 100) / 100.0f * 2.f * M_PI;
                        float size = rand() % 100 / 200.0f + 0.5;
                        size *= size;
                        int type = rand() % 2;
                        debris.push_back(
                            {glm::vec2{0.0003, 0.0003} * size,
                             size * (1.0f * type + 0.1f),
                             t.vel + glm::vec2{cos(velangle), sin(velangle)} *
                                         (rand() % 100 / 2000.0f),
                             t.pos + glm::vec2(x + rand() % 100 / 100.0,
                                               y + rand() % 100 / 100.0) *
                                         sim.h,
                             rand() % 100 / 100.0f * 2.0f * (float)M_PI,
                             rand() % 100 - 50.0f, &(textures[type + 1])});
                      }
                    }
                    sim.setGrids(gridC, 1.0);
                  }
                }
                sim.mg.updateFields(sim.flag);
              }
              return explode;
            }),
        end(torpedos));

    for (auto &exp : explosions) {
      for (auto &ag : swarm.agents) {
        if (length(exp.pos - ag.pos) < explosionDiam * 0.9 && exp.age < 0.08f)
          ag.pos = glm::vec2(-1, -1);
      }
      for (auto &ag : playerShips) {
        if (length(exp.pos - ag.pos) < explosionDiam * 0.9 && exp.age < 0.08f &&
            exp.player != ag.player)
          ag.pos = glm::vec2(-1, -1);
      }
    }

    explosions.erase(
        remove_if(begin(explosions), end(explosions),
                  [=](auto &t) { return processExplosion(t, simTime); }),
        end(explosions));

    debris.erase(removeOutOfBounds(begin(debris), end(debris), sim.width,
                                   sim.height, sim, sim.h),
                 end(debris));
    debris.erase(remove_if(begin(debris), end(debris),
                           [](FloatingItem &d) { return rand() % 1000 == 42; }),
                 end(debris));

    torpedos.erase(removeOutOfBounds(begin(torpedos), end(torpedos), sim.width,
                                     sim.height, sim, sim.h),
                   end(torpedos));

    swarm.update(playerShips[0], sim.flag, sim.h);
  }
  double sim_t2 = dtime();
  lastSimulationTime = (sim_t2 - sim_t1) / simulationSteps;
  double thisFrameTime = dtime();
  smoothedFrameRate =
      0.95 * smoothedFrameRate + 0.05 / (thisFrameTime - lastFrameTime);
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
  if (playerCount == 4) {
    xsplits = 2;
    ysplits = 2;
  } else {
    xsplits = playerCount;
  }

  int displayWidth = ImGui::GetIO().DisplaySize.x;
  int displayHeight = ImGui::GetIO().DisplaySize.y;
  int renderWidth = displayWidth / xsplits * 0.99;
  int renderHeight = (displayHeight - 50) / ysplits;
  int renderOriginX = renderWidth;
  int renderOriginY = 0;

  bool p_open;

  double graphicsT1 = dtime();

  for (int pid = 0; pid < players.size(); pid++) {
    DrawTracers::playerTracersAdd(
        pid,
        ((playerShips[pid].pos - glm::vec2{cos(playerShips[pid].rotation),
                                           sin(playerShips[pid].rotation)} *
                                     playerShips[pid].size.x * 0.2f)) /
                sim.h +
            glm::vec2(0.75f, 0.2f));
  }
  DrawTracers::updateTracers(sim.getVX(), sim.getVY(), sim.getFlag(), sim.width,
                             sim.height, simTime, sim.pwidth);

  for (unsigned int i = 0; i < players.size(); i++) {
    renderOriginX = renderWidth * (i % xsplits * 1.01);
    renderOriginY = renderHeight * (i / xsplits) * 1.01;

    ImGui::SetNextWindowPos(ImVec2(
        renderOriginX + 2,
        displayHeight - (i / xsplits + 1) * ((displayHeight - 55) / ysplits)));

    ImGui::SetNextWindowSize(ImVec2(200, 100));
    ImGui::Begin(("Player" + to_string(i)).c_str(), &p_open,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    ImGui::Text("Player %d", i);
    ImGui::Separator();
    ImGui::Text("Kills: %d, Deaths: %d", players[i].kills, players[i].deaths);
    ImGui::Text("Torpedos: %.0f", players[i].torpedosLoaded);
    ImGui::End();

    GL_CALL(
        glViewport(renderOriginX, renderOriginY, renderWidth, renderHeight));

    GL_CALL(glEnable(GL_FRAMEBUFFER_SRGB));

    GL_CALL(glEnable(GL_BLEND));
    GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE));

    // Projection-View-Matrix
    glm::mat4 PVM(1.0f);
    // Projection
    // PVM = glm::translate(PVM, glm::vec3(0.0f, -0.4f, 0.0f));

    PVM = glm::scale(
        PVM, glm::vec3(2 * scale * xsplits,
                       2 * scale * xsplits * (float)renderWidth / renderHeight,
                       1.0f));
    // View
    // PVM = glm::rotate(PVM, playerShips[i].rotation - glm::half_pi<float>(),
    //                  glm::vec3(0.0f, 0.0f, -1.0f));
    PVM = glm::translate(PVM, glm::vec3(-playerShips[i].pos, 0.0f));

    //  PVM = glm::translate(PVM,
    //                     glm::vec3(-0.5, -0.5 * renderHeight / renderWidth,
    //                     0.0));

    Draw2DBuf::draw_mag(sim.getVX(), sim.getVY(), sim.width, sim.height, PVM,
                        sim.pwidth);
    Draw2DBuf::draw_flag(rock_texture, sim.getFlag(), sim.width, sim.height,
                         PVM, sim.pwidth);

    DrawTracers::draw(sim.width, sim.height, PVM, sim.pwidth);

    DrawFloatingItems::draw(&*begin(debris), &*end(debris), PVM);
    DrawFloatingItems::draw(&*begin(swarm.agents), &*end(swarm.agents), PVM);
    DrawFloatingItems::draw(&*begin(torpedos), &*end(torpedos), PVM);

    DrawFloatingItems::draw(&*begin(playerShips), &*end(playerShips), PVM);
    DrawFloatingItems::draw(&*begin(explosions), &*end(explosions), PVM);
  }
  double graphicsT2 = dtime();

  ImGui::SetNextWindowPos(ImVec2(10, 10));
  ImGui::SetNextWindowSize(ImVec2(400, 40));
  ImGui::Begin("SideBar", &p_open,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

  ImGui::TextWrapped("gfx: %.0f ms, %.0f FPS, %d sims/frame",
                     (graphicsT2 - graphicsT1) * 1000.0, smoothedFrameRate,
                     simulationSteps);
  ImGui::End();
}
