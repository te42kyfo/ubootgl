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
    simTime = 0.0;
    iterationCounter = 0;
    scale = 1.0;
  }

  void loop() {
    double t1 = dtime();
    simTime = 0;
    while (dtime() - t1 < 0.02) {
      sim.step();
      simTime += sim.dt;
    }
    iterationCounter++;
  }

  void draw() {
    SDL_GL_MakeCurrent(vis.window, vis.gl_context);
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
  }

  double frame_rate;
  float scale;
  float simTime;
  uint iterationCounter;
  SdlGl vis;
  Simulation sim;
};
