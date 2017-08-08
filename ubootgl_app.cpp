#include "ubootgl_app.hpp"
#include <vector>
#include "dtime.hpp"
#include "gl_error.hpp"

using namespace std;

void UbootGlApp::loop() {
  double t1 = dtime();
  simTime = 0;
  simIterationCounter = 0;
  while (dtime() - t1 < 0.02) {
    sim.step();
    simTime += sim.dt;
    simIterationCounter++;
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

  ImGui::Begin("Example: Fixed Overlay", &p_open,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_ShowBorders);
  ImGui::TextWrapped(sim.diag.str().c_str());
  ImGui::Separator();
  ImGui::TextWrapped("FPS: %.1f, %d sims/frame", smoothedFrameRate,
                     simIterationCounter);
  ImGui::End();

  GL_CALL(glViewport(renderOriginX, renderOriginY, renderWidth, renderHeight));
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  Draw2DBuf::draw_mag(sim.getVX(), sim.getVY(), sim.width, sim.height,
                      renderWidth, renderHeight, scale);
  /*DrawStreamlines::draw(sim.getVX(), sim.getVY(), sim.width, sim.height,
                        pixelWidth, pixelHeight, scale);
  */

  /*  vector<float> VX(sim.width * sim.height);
  vector<float> VY(sim.width * sim.height);
  vector<float> flag(sim.width * sim.height);
  for (int y = 0; y < sim.height; y++) {
    for (int x = 0; x < sim.width; x++) {
      VX[y * sim.width + x] = -0.1 * (sim.height / 2 - y);
      VY[y * sim.width + x] = 0.1 * (sim.width / 2 - x);
      flag[y * sim.width + x] = 1;
    }
    }*/

  DrawTracers::draw(sim.getVX(), sim.getVY(), sim.getFlag(), sim.width,
                    sim.height, renderWidth, renderHeight, scale, simTime,
                    sim.pwidth / (sim.width - 1));

  double thisFrameTime = dtime();
  smoothedFrameRate =
      0.95 * smoothedFrameRate + 0.05 / (thisFrameTime - lastFrameTime);
  lastFrameTime = thisFrameTime;
}
