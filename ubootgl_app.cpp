#include "ubootgl_app.hpp"
#include <GL/glew.h>
#include <glm/gtx/transform.hpp>
#include <glm/vec2.hpp>
#include <vector>
#include "dtime.hpp"
#include "gl_error.hpp"

using namespace std;

void UbootGlApp::loop() {
  double updateTime = dtime();
  double timeDelta = updateTime - lastKeyUpdate;
  if (keysPressed[0]) playerPosition.x -= 0.2 * timeDelta;
  if (keysPressed[1]) playerPosition.x += 0.2 * timeDelta;
  if (keysPressed[2]) playerPosition.y -= 0.2 * timeDelta;
  if (keysPressed[3]) playerPosition.y += 0.2 * timeDelta;
  lastKeyUpdate = updateTime;

  double t1 = dtime();
  simTime = 0;
  simIterationCounter = 0;
  while (dtime() - t1 < 0.01) {
    sim.step();
    simTime += sim.dt;
    simIterationCounter++;
  }
}

void UbootGlApp::handleKey(SDL_KeyboardEvent event) {
  switch (event.keysym.sym) {
    case SDLK_RIGHT:
      keysPressed[0] = event.state;
      break;
    case SDLK_LEFT:
      keysPressed[1] = event.state;
      break;
    case SDLK_UP:
      keysPressed[2] = event.state;
      break;
    case SDLK_DOWN:
      keysPressed[3] = event.state;
      break;
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

  glm::vec2 origin = playerPosition;

  // Projection-View-Matrix
  glm::mat4 PVM(1.0f);
  // Projection
  PVM = glm::scale(PVM,
                   glm::vec3(2.0f * scale * renderHeight / (float)renderWidth,
                             2.0f * scale, 1.0f));
  // View
  PVM = glm::translate(PVM, glm::vec3(origin, 0.0f));

  Draw2DBuf::draw_mag(sim.getVX(), sim.getVY(), sim.width, sim.height, PVM);

  Draw2DBuf::draw_flag(rock_texture, sim.getFlag(), sim.width, sim.height, PVM);

  DrawTracers::draw(sim.getVX(), sim.getVY(), sim.getFlag(), sim.width,
                    sim.height, PVM, simTime, sim.pwidth / (sim.width - 1));

  DrawFloatingItems::draw(sim.floatingItems, PVM);

  double thisFrameTime = dtime();
  smoothedFrameRate =
      0.9 * smoothedFrameRate + 0.1 / (thisFrameTime - lastFrameTime);
  lastFrameTime = thisFrameTime;
}
