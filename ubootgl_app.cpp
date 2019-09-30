#include "ubootgl_app.hpp"
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
void respawnOutOfBounds(T begin, T end, int width, int height,
                        Single2DGrid const &flag, float h) {

  static auto disx = std::uniform_real_distribution<float>(0.0f, 1.0f);
  static auto disy =
      std::uniform_real_distribution<float>(0.0f, (float)height / width);
  static std::default_random_engine gen(std::random_device{}());

  for (auto it = begin; it != end; it++) {
    glm::vec2 gridPos = it->pos / h + 0.5f;
    while (gridPos.x >= width - 2 || gridPos.x <= 1.0 ||
           gridPos.y >= height - 2 || gridPos.y <= 1.0 ||
           flag(gridPos.x, gridPos.y) < 0.01) {
      it->pos = glm::vec2(disx(gen), disy(gen));
      it->vel = {0.0f, 0.0f};
      it->angVel = 0;
      gridPos = it->pos / h + 0.5f;
    }
  }
}
template <typename T>
auto removeOutOfBounds(T begin, T end, int width, int height,
                       Single2DGrid const &flag, float h) {

  return remove_if(begin, end, [&](auto &it) {
    glm::vec2 gridPos = it.pos / h + 0.5f;
    return (gridPos.x >= width - 2 || gridPos.x <= 1.0 ||
            gridPos.y >= height - 2 || gridPos.y <= 1.0 ||
            flag(gridPos.x, gridPos.y) < 0.01);
  });
}

void UbootGlApp::loop() {
  double updateTime = dtime();
  double timeDelta = updateTime - lastKeyUpdate;
  if (keysPressed[SDLK_RIGHT])
    ship.rotation -= 4.0 * timeDelta;
  if (keysPressed[SDLK_LEFT])
    ship.rotation += 4.0 * timeDelta;
  if (keysPressed[SDLK_UP])
    ship.force += 6.0f * glm::vec2(cos(ship.rotation), sin(ship.rotation));

  if (keysPressed[SDLK_DOWN])
    ship.force -= 3.0f * glm::vec2(cos(ship.rotation), sin(ship.rotation));
  if (keysPressed[SDLK_PLUS])
    scale *= pow(2, timeDelta);
  if (keysPressed[SDLK_MINUS])
    scale *= pow(0.5, timeDelta);

  if (keysPressed[SDLK_SPACE] || keysPressed[SDLK_a]) {
    torpedos.push_back(
        {glm::vec2{0.0003, 0.002}, 0.2,
         ship.vel + glm::vec2{cos(ship.rotation), sin(ship.rotation)} * 0.2f,
         ship.pos, ship.rotation, ship.angVel, &(textures[4])});
    keysPressed[SDLK_SPACE] = false;
  }

  lastKeyUpdate = updateTime;

  double t1 = dtime();

  simTime = 0;
  simIterationCounter = 0;
  while (dtime() - t1 < 0.01) {

    sim.step();

    sim.advectFloatingItems(&ship, &ship + 1);
    sim.advectFloatingItems(&*begin(debris), &*end(debris));
    sim.advectFloatingItems(&*begin(swarm.agents), &*end(swarm.agents));
    sim.advectFloatingItems(&*begin(torpedos), &*end(torpedos));

    simTime += sim.dt;
    simIterationCounter++;
  }

  respawnOutOfBounds(&*begin(swarm.agents), &*end(swarm.agents), sim.width,
                     sim.height, sim.flag, sim.h);
  respawnOutOfBounds(&ship, &ship + 1, sim.width, sim.height, sim.flag, sim.h);

  // sim.sinks.clear();

  float explosionDiam = 0.005;
  torpedos.erase(
      remove_if(
          begin(torpedos), end(torpedos),
          [=](auto &t) {
            bool explode = processTorpedo(t, &*begin(swarm.agents),
                                          &*end(swarm.agents), simTime);
            if (explode) {
              explosions.push_back({glm::vec2{0.01, 0.01}, 0.01, t.vel, t.pos,
                                    rand() % 100 * 100.0f * 2.0f * 3.14f, 0,
                                    &(textures[5])});
              explosions.back().age = 0.01f;

              sim.sinks.push_back(glm::vec3(t.pos, 100.0f));
              float diam = explosionDiam / sim.h;
              for (int y = -diam; y <= diam; y++) {
                for (int x = -diam; x <= diam; x++) {
                  auto gridC = (t.pos + glm::vec2(x, y) * sim.h) / sim.h + 0.5f;
                  if (x * x + y * y > diam * diam)
                    continue;
                  if (sim.flag(gridC) < 1.0) {
                    for (int i = 0; i < 10; i++) {
                      float velangle =
                          orientedAngle(t.vel, glm::vec2{0.0f, 1.0f}) +
                          (rand() % 100) / 100.0f * 2.f * M_PI;
                      float size = rand() % 100 / 50.0f + 0.1f;
                      int type = rand() % 2;
                      debris.push_back(
                          {glm::vec2{0.0003, 0.0003} * size,
                           size * (0.4f * type + 0.2f),
                           t.vel + glm::vec2{cos(velangle), sin(velangle)} *
                                       (rand() % 100 / 800.0f),
                           t.pos + glm::vec2(x, y) * sim.h,
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
      if (exp.age > 0.02 && exp.age < 0.06 &&
          length(exp.pos - ag.pos) < explosionDiam * 0.8f)
        ag.pos = glm::vec2(-1, -1);
    }
  }
  explosions.erase(
      remove_if(begin(explosions), end(explosions),
                [=](auto &t) { return processExplosion(t, simTime); }),
      end(explosions));

  debris.erase(removeOutOfBounds(begin(debris), end(debris), sim.width,
                                 sim.height, sim.flag, sim.h),
               end(debris));
  torpedos.erase(removeOutOfBounds(begin(torpedos), end(torpedos), sim.width,
                                   sim.height, sim.flag, sim.h),
                 end(torpedos));

  swarm.update(ship, sim.flag, sim.h);
}

void UbootGlApp::handleKey(SDL_KeyboardEvent event) {

  switch (event.keysym.sym) {
  case SDLK_q:
    exit(0);
  default:
    keysPressed[event.keysym.sym] = event.state;
  }
}

void UbootGlApp::draw() {
  bool p_open;

  int displayWidth = ImGui::GetIO().DisplaySize.x;
  int displayHeight = ImGui::GetIO().DisplaySize.y;
  int renderWidth = displayWidth;
  int renderHeight = displayHeight;
  int renderOriginX = 0;
  int renderOriginY = 0;

  int sideBarSize = 250;
  if (1.0 * (displayWidth - 250) / (displayHeight - 150) >
      1.0 * sim.width / sim.height) {
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(sideBarSize - 10, displayHeight - 10));
    renderOriginX = sideBarSize;
    renderWidth = displayWidth - sideBarSize;
  } else {
    sideBarSize = 150;
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(displayWidth - 10, sideBarSize - 10));
    renderOriginY = 0;
    renderHeight = displayHeight - sideBarSize;
  }

  ImGui::Begin("SideBar", &p_open,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_ShowBorders);
  ImGui::TextWrapped("%s", sim.diag.str().c_str());
  ImGui::Separator();
  ImGui::TextWrapped("FPS: %.1f, %d sims/frame", smoothedFrameRate,
                     simIterationCounter);

  GL_CALL(glViewport(renderOriginX, renderOriginY, renderWidth, renderHeight));
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
  GL_CALL(glEnable(GL_FRAMEBUFFER_SRGB));

  GL_CALL(glEnable(GL_BLEND));
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE));

  glm::vec2 origin = ship.pos;

  // Projection-View-Matrix
  glm::mat4 PVM(1.0f);
  // Projection
  PVM = glm::scale(PVM, glm::vec3(2 * scale,
                                  2 * scale * (float)renderWidth / renderHeight,
                                  1.0f));
  // View

  PVM = glm::translate(PVM, glm::vec3(-origin, 0.0f));
  //  PVM = glm::translate(PVM,
  //                     glm::vec3(-0.5, -0.5 * renderHeight / renderWidth,
  //                     0.0));

  double graphicsT1 = dtime();

  Draw2DBuf::draw_mag(sim.getVX(), sim.getVY(), sim.width, sim.height, PVM);

  Draw2DBuf::draw_flag(rock_texture, sim.getFlag(), sim.width, sim.height, PVM);

  DrawTracers::draw(sim.getVX(), sim.getVY(), sim.getFlag(), sim.width,
                    sim.height, PVM, simTime, sim.pwidth / (sim.width - 1));

  DrawFloatingItems::draw(&*begin(debris), &*end(debris), PVM);
  DrawFloatingItems::draw(&*begin(swarm.agents), &*end(swarm.agents), PVM);
  DrawFloatingItems::draw(&*begin(torpedos), &*end(torpedos), PVM);

  DrawFloatingItems::draw(&ship, &ship + 1, PVM);
  DrawFloatingItems::draw(&*begin(explosions), &*end(explosions), PVM);
  double graphicsT2 = dtime();
  double thisFrameTime = dtime();
  smoothedFrameRate =
      0.9 * smoothedFrameRate + 0.1 / (thisFrameTime - lastFrameTime);
  lastFrameTime = thisFrameTime;

  ImGui::TextWrapped("graphics time: %10.1f ms",
                     (graphicsT2 - graphicsT1) * 1000.0);

  ImGui::End();
}
