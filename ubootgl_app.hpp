#pragma once

#include <iostream>
#include <vector>
#include "draw_2dbuf.hpp"
#include "draw_streamlines.hpp"
#include "draw_text.hpp"
#include "draw_tracers.hpp"
#include "dtime.hpp"
#include "gl_error.hpp"
#include "imgui/imgui.h"
#include "sdl_gl.hpp"
#include "simulation.hpp"

class UbootGlApp {
 public:
  UbootGlApp() : sim("level.png", 1.0, 1.0f) {
    Draw2DBuf::init();
    DrawText::init();
    DrawStreamlines::init();
    DrawTracers::init();
    scale = 1.0;
  }

  void loop() {
    double t1 = dtime();
    simTime = 0;
    simIterationCounter = 0;
    while (dtime() - t1 < 0.02) {
      sim.step();
      simTime += sim.dt;
      simIterationCounter++;
    }
  }

  void draw() {
    bool p_open;
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(250, ImGui::GetIO().DisplaySize.y - 10));
    ImGui::Begin("Example: Fixed Overlay", &p_open,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings |
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

  double lastFrameTime = 0;
  double smoothedFrameRate = 0;
  float scale;
  float simTime = 0.0;
  uint simIterationCounter = 0;
  SdlGl vis;
  Simulation sim;
};
