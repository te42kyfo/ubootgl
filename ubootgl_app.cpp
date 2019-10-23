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

  simulationSteps = 1; // min(100.0, max(1.0, 0.02 / lastSimulationTime));

  float timestep = 0.1f / max(1.0, smoothedFrameRate * simulationSteps);

  double updateTime = dtime();
  double timeDelta = (updateTime - lastKeyUpdate) / simulationSteps;

  double sim_t1 = dtime();
  for (int simStep = 0; simStep < simulationSteps; simStep++) {

    for (int pid = 0; pid < (int)players.size(); pid++) {

      if (keysPressed[key_map[{pid, CONTROLS::TURN_CLOCKWISE}]])
        playerShips[pid].rotation -= 4.0 * timeDelta;
      if (keysPressed[key_map[{pid, CONTROLS::TURN_COUNTERCLOCKWISE}]])
        playerShips[pid].rotation += 4.0 * timeDelta;
      if (keysPressed[key_map[{pid, CONTROLS::THRUST_FORWARD}]])
        playerShips[pid].force +=
            8.0f * glm::vec2(cos(playerShips[pid].rotation),
                             sin(playerShips[pid].rotation));
      if (keysPressed[key_map[{pid, CONTROLS::THRUST_BACKWARD}]])
        playerShips[pid].force -=
            3.0f * glm::vec2(cos(playerShips[pid].rotation),
                             sin(playerShips[pid].rotation));
      if (keysPressed[key_map[{pid, CONTROLS::LAUNCH_TORPEDO}]]) {
        if (players[pid].torpedoCooldown < 0.0001 &&
            players[pid].torpedosLoaded > 1.0) {
          /*torpedos.push_back({glm::vec2{0.002, 0.0004}, 0.15,
                            playerShips[pid].vel +
                                glm::vec2{cos(playerShips[pid].rotation),
                                          sin(playerShips[pid].rotation)} *
                                    0.2f,
                            playerShips[pid].pos, playerShips[pid].rotation,
                            playerShips[pid].angVel, &(textures[4]), pid});

*/

          auto newTorpedo = registry.create();
          registry.assign<CoItem>(newTorpedo, glm::vec2{0.002, 0.0004},
                                  playerShips[pid].pos,
                                  playerShips[pid].rotation);
          registry.assign<CoKinematics>(
              newTorpedo, 0.15,
              playerShips[pid].vel + glm::vec2{cos(playerShips[pid].rotation),
                                               sin(playerShips[pid].rotation)} *
                                         0.2f,
              playerShips[pid].angVel);
          registry.assign<CoSprite>(newTorpedo, &textures[4], 0.0f);

          registry.assign<entt::tag<"torpedo_tex"_hs>>(newTorpedo);

          players[pid].torpedoCooldown = 0.014;
          players[pid].torpedosLoaded -= 1.0;
          players[pid].torpedosFired++;
        }
      }
      players[pid].torpedoCooldown =
          max(0.0f, players[pid].torpedoCooldown - timestep);
      players[pid].torpedosLoaded =
          min(10.0f, players[pid].torpedosLoaded + 14.0f * timestep);
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

    if (playerCount != players.size()) {
      cout << "respawn, " << playerCount << " " << players.size() << "\n";
      players.clear();
      playerShips.clear();
      for (unsigned int p = 0; p < playerCount; p++) {
        cout << p << "\n";
        playerShips.push_back({glm::vec2{0.008, 0.002}, 1.3, glm::vec2(0, 0),
                               glm::vec2(-0.5, -0.5), 0.0, 0.0, &textures[0],
                               (int)p});
        players.push_back(Player(p));
      }
    }

    lastKeyUpdate = updateTime;

    simTime = 0;

    sim.step(timestep);

    // sim.advectFloatingItems(&*begin(playerShips), &*end(playerShips));
    // sim.advectFloatingItems(&*begin(debris), &*end(debris));
    // sim.advectFloatingItems(&*begin(swarm.agents), &*end(swarm.agents));
    // sim.advectFloatingItems(&*begin(torpedos), &*end(torpedos));
    sim.advectFloatingItems(registry);

    simTime += sim.dt;

    /*respawnOutOfBounds(&*begin(swarm.agents), &*end(swarm.agents), sim.width,
      sim.height, sim, sim.h);*/
    respawnOutOfBounds(&*begin(playerShips), &*end(playerShips), sim.width,
                       sim.height, sim, sim.h);

    // sim.sinks.clear();
    /*
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
                      for (int i = 0; i < 20; i++) {
                        float velangle =
                            orientedAngle(t.vel, glm::vec2{0.0f, 1.0f}) +
                            (rand() % 100) / 100.0f * 2.f * M_PI;
                        float size = rand() % 100 / 100.0f + 0.6;
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
      for (int pid = 0; pid < (int)players.size(); pid++) {
        if (length(exp.pos - playerShips[pid].pos) <
                explosionDiam * 0.5 +
                    (playerShips[pid].size.x + playerShips[pid].size.y) * 0.5 &&
            exp.age < 0.08f && exp.player != pid &&
            players[pid].deathtimer == 0.0f) {

          explosions.push_back({glm::vec2{0.02, 0.02}, 0.01,
                                playerShips[pid].vel, playerShips[pid].pos,
                                rand() % 100 * 100.0f * 2.0f * 3.14f, 0,
                                &(textures[5]), pid});
          explosions.back().age = 0.02f;
          sim.sinks.push_back(glm::vec3(playerShips[pid].pos, 200.0f));

          players[pid].deathtimer = 0.08f;
          players[pid].deaths++;
          players[exp.player].kills++;
        }
      }
    }
*/
    for (auto &p : players) {
      if (p.deathtimer > 0.0f) {
        playerShips[p.id].vel = glm::vec2(0.0f, 0.0f);
        p.deathtimer -= simTime;
        if (p.deathtimer < 0.0f) {
          playerShips[p.id].pos = glm::vec2(-1.0f, -1.0f);
          p.deathtimer = 0.0f;
        }
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
            (sim.pwidth / (sim.width)));
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

    //    DrawFloatingItems::draw(&*begin(debris), &*end(debris), PVM, 1.0f);
    // DrawFloatingItems::draw(&*begin(swarm.agents), &*end(swarm.agents), PVM,
    //                        1.0f);
    //   DrawFloatingItems::draw(&*begin(torpedos),
    //   &*end(torpedos), PVM, 1.0f);

    DrawFloatingItems::draw(registry, PVM, 1.0f);

    // DrawFloatingItems::draw(&*begin(playerShips),
    // &*end(playerShips), PVM,
    //                        1.0f);
    // DrawFloatingItems::draw(&*begin(explosions),
    // &*end(explosions), PVM, 1.0f);
  }

  renderOriginX = displayWidth * 0.4;
  renderOriginY = displayHeight * 0.0;
  renderWidth = displayWidth * 0.2;
  renderHeight = displayWidth * 0.2;

  GL_CALL(glViewport(renderOriginX, renderOriginY, renderWidth, renderHeight));

  GL_CALL(glEnable(GL_FRAMEBUFFER_SRGB));

  GL_CALL(glEnable(GL_BLEND));
  GL_CALL(glBlendFunc(GL_ONE, GL_ZERO));

  // Projection-View-Matrix
  glm::mat4 PVM(1.0f);
  PVM = glm::scale(
      PVM,
      glm::vec3(1.0f, 1.0f * (float)renderWidth / renderHeight, 1.0f) * 2.0f);
  PVM = glm::translate(PVM, glm::vec3(-0.5f, -0.5f, 0.0f));
  Draw2DBuf::draw_flag(textures[6], sim.getFlag(), sim.width, sim.height, PVM,
                       sim.pwidth);

  DrawFloatingItems::draw(&*begin(torpedos), &*end(torpedos), PVM, 4.0f);
  DrawFloatingItems::draw(&*begin(playerShips), &*end(playerShips), PVM, 5.0f);
  DrawFloatingItems::draw(&*begin(explosions), &*end(explosions), PVM, 3.0f);

  ImGui::SetNextWindowPos(ImVec2(10, 10));
  ImGui::SetNextWindowSize(ImVec2(400, 50));
  ImGui::Begin("SideBar", &p_open,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

  ImGui::TextWrapped("Total: %.1f, %.1f, %.1f ", frameTimes.avg(),
                     frameTimes.high1pct(), frameTimes.largest());
  ImGui::TextWrapped("GFX: %.1f, %.1f, %.1f ", gfxTimes.avg(),
                     gfxTimes.high1pct(), gfxTimes.largest());

  ImGui::End();

  double graphicsT2 = dtime();
  gfxTimes.add((graphicsT2 - graphicsT1) * 1000.0);
}
