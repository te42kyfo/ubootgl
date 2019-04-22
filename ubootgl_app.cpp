#include "ubootgl_app.hpp"
#include "dtime.hpp"
#include "gl_error.hpp"
#include "torpedo.hpp"
#include <GL/glew.h>

#include <glm/gtx/transform.hpp>
#include <glm/vec2.hpp>
#include <vector>

using namespace std;

void UbootGlApp::loop() {
  double updateTime = dtime();
  double timeDelta = updateTime - lastKeyUpdate;
  if (keysPressed[SDLK_RIGHT])
    ship.rotation -= 3.0 * timeDelta;
  if (keysPressed[SDLK_LEFT])
    ship.rotation += 3.0 * timeDelta;
  if (keysPressed[SDLK_UP])
    ship.force += 6.0f * glm::vec2(cos(ship.rotation), sin(ship.rotation));
  if (keysPressed[SDLK_DOWN])
    ship.force -= 3.0f * glm::vec2(cos(ship.rotation), sin(ship.rotation));
  if (keysPressed[SDLK_PLUS])
    scale *= pow(2, timeDelta);
  if (keysPressed[SDLK_MINUS])
    scale *= pow(0.5, timeDelta);

  if (keysPressed[SDLK_SPACE]) {
    torpedos.push_back(
        {glm::vec2{0.0003, 0.002}, 0.07,
         ship.vel + glm::vec2{cos(ship.rotation), sin(ship.rotation)} * 1.0f,
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

  torpedos.erase(
      remove_if(
          begin(torpedos), end(torpedos),
          [=](auto &t) {
            bool explode = processTorpedo(t, &*begin(swarm.agents),
                                          &*end(swarm.agents), simTime);
            if (explode) {
              explosions.push_back({glm::vec2{0.01, 0.01}, 0.01, t.vel, t.pos,
                                    0, 0, &(textures[5])});
              for (int i = 0; i < 10; i++) {
                float velangle = orientedAngle(t.vel, glm::vec2{0.0f, 1.0f});
                debris.push_back(
                                 {glm::vec2{0.0003, 0.0003}*(rand()%10/5.0f+0.2f), 0.8f,
                     t.vel + glm::vec2{cos(velangle + (i - 5) * 0.2 +rand()%10/10.0), sin(velangle + (i - 5) * 0.2)} *
                                 0.2f,
                     t.pos, 0.0f, 0.0f, &(textures[i % 2 + 1])});
              }
            }
            return explode;
          }),
      end(torpedos));

  swarm.nnUpdate(ship, sim.flag, sim.h);
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
  ImGui::End();

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

  Draw2DBuf::draw_mag(sim.getVX(), sim.getVY(), sim.width, sim.height, PVM);

  Draw2DBuf::draw_flag(rock_texture, sim.getFlag(), sim.width, sim.height, PVM);

  DrawTracers::draw(sim.getVX(), sim.getVY(), sim.getFlag(), sim.width,
                    sim.height, PVM, simTime, sim.pwidth / (sim.width - 1));

  DrawFloatingItems::draw(&ship, &ship + 1, PVM);
  DrawFloatingItems::draw(&*begin(debris), &*end(debris), PVM);
  DrawFloatingItems::draw(&*begin(swarm.agents), &*end(swarm.agents), PVM);
  DrawFloatingItems::draw(&*begin(torpedos), &*end(torpedos), PVM);
  DrawFloatingItems::draw(&*begin(explosions), &*end(explosions), PVM);

  double thisFrameTime = dtime();
  smoothedFrameRate =
      0.9 * smoothedFrameRate + 0.1 / (thisFrameTime - lastFrameTime);
  lastFrameTime = thisFrameTime;
}
