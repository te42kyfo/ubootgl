#include "ubootgl_app.hpp"
#include "dtime.hpp"
#include "gl_error.hpp"

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
  ImGui::SetNextWindowPos(ImVec2(10, 10));
  ImGui::SetNextWindowSize(ImVec2(250, ImGui::GetIO().DisplaySize.y - 10));
  ImGui::Begin("Example: Fixed Overlay", &p_open,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                   ImGuiWindowFlags_ShowBorders);

  ImGui::TextWrapped(sim.diag.str().c_str());
  ImGui::Separator();
  ImGui::TextWrapped("FPS: %.1f, %d sims/frame", smoothedFrameRate,
                     simIterationCounter);

  ImGui::End();
  GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  int pixelWidth = ImGui::GetIO().DisplaySize.x;
  int pixelHeight = ImGui::GetIO().DisplaySize.y;
  // Draw2DBuf::draw_mag(sim.getVX(), sim.getVY(), sim.width, sim.height,
  //                    pixelWidth, pixelHeight, scale);
  /*DrawStreamlines::draw(sim.getVX(), sim.getVY(), sim.width, sim.height,
                        pixelWidth, pixelHeight, scale);
  */
  DrawTracers::draw(sim.getVX(), sim.getVY(), sim.getFlag(), sim.width,
                    sim.height, pixelWidth, pixelHeight, scale, simTime,
                    sim.pwidth / (sim.width - 1));

  double thisFrameTime = dtime();
  smoothedFrameRate =
      0.95 * smoothedFrameRate + 0.05 / (thisFrameTime - lastFrameTime);
  lastFrameTime = thisFrameTime;
}
