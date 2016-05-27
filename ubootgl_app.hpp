#pragma once

#include <iostream>
#include <vector>
#include "draw_2dbuf.hpp"
#include "draw_text.hpp"
#include "dtime.hpp"
#include "gl_error.hpp"
#include "sdl_gl.hpp"
#include "simulation.hpp"

class UbootGlApp {
 public:
  UbootGlApp() : sim(500, 500) {
    vis.initDisplay(1);
    vis.setViewport(800, 600);
    Draw2DBuf::init();
    DrawText::init();
    lastFrameTime = dtime();
    iteration_counter = 0;
    scale = 1.0;
  }

  void handle_keys(const SDL_Event& e) {
    switch (e.key.keysym.sym) {
      case SDLK_PLUS:
        scale *= 1.03;
        break;
      case SDLK_MINUS:
        scale /= 1.03;
        break;
    }
  }
  void handle_mouse(const SDL_Event& e) {}

  void loop() {
    double t1 = dtime();
    while (dtime() - t1 < 0.02) {
      sim.step();
    }

    draw();
    iteration_counter++;
    double thisFrameTime = dtime();
    if (thisFrameTime - lastFrameTime > 0.5) {
      frame_rate = iteration_counter / (thisFrameTime - lastFrameTime);
      iteration_counter = 0;
      lastFrameTime = thisFrameTime;
    }
  }
  void draw() {
    SDL_GL_MakeCurrent(vis.windows[0], vis.gl_context);
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    Draw2DBuf::draw(sim.getVX(), sim.width, sim.height, vis.pixel_width,
                    vis.pixel_height, scale);

    DrawText::draw(std::to_string((int)frame_rate), -1, 0.9, 0.05,
                   vis.pixel_width, vis.pixel_height);
    SDL_GL_SwapWindow(vis.windows[0]);
  }
  double frame_rate;
  float scale;
  double lastFrameTime;
  uint iteration_counter;
  SdlGl vis;
  Simulation sim;
};
