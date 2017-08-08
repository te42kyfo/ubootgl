#pragma once

#include <iostream>
#include <vector>
#include "draw_2dbuf.hpp"
#include "draw_streamlines.hpp"
#include "draw_text.hpp"
#include "draw_tracers.hpp"

#include "imgui/imgui.h"
#include "sdl_gl.hpp"
#include "simulation.hpp"

class UbootGlApp {
 public:
  UbootGlApp() : sim("level.png", 1.0, 0.5f) {
    Draw2DBuf::init();
    DrawText::init();
    DrawStreamlines::init();
    DrawTracers::init();
    scale = 1.0;
  }

  void loop();
  void draw();

  double lastFrameTime = 0;
  double smoothedFrameRate = 0;
  float scale;
  float simTime = 0.0;
  uint simIterationCounter = 0;
  SdlGl vis;
  Simulation sim;
};
