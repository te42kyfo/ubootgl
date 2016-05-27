#pragma once

#include <iostream>
#include <vector>
#include "draw_2dbuf.hpp"
#include "draw_text.hpp"
#include "dtime.hpp"
#include "gl_error.hpp"
#include "sdl_gl.hpp"

class UbootGlApp {
 public:
  UbootGlApp() : data(2000 * 2000) {
    vis.initDisplay(1);
    vis.setViewport(800, 600);
    Draw2DBuf::init();
    DrawText::init();
    lastFrameTime = dtime();
    iteration_counter = 0;
    scale = 1.0;

    for (int i = 0; i < 1000 * 1000; i++) {
      data[i] = sin(static_cast<int>(i / 1000) * 0.5) + sin(i % 1000 * 0.9) +
                sin(i % 1000 * 2.3);
    }
  }

  void handle_keys(const SDL_Event& e) {
    switch (e.key.keysym.sym) {
      case SDLK_PLUS:
        scale *= 1.01;
        break;
      case SDLK_MINUS:
        scale /= 1.01;
        break;
    }
  }
  void handle_mouse(const SDL_Event& e) {}

  void loop() {
    // double t1 = dtime();
    //    while (dtime() - t1 < 0.03) {
    // sim.step();
    //}

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

    Draw2DBuf::draw(data.data(), 1000, 1000, vis.pixel_width, vis.pixel_height,
                    scale);

    DrawText::draw(std::to_string((int)frame_rate), -1, 0.9, 0.05,
                   vis.pixel_width, vis.pixel_height);
    SDL_GL_SwapWindow(vis.windows[0]);
  }
  double frame_rate;
  std::vector<float> data;
  float scale;
  double lastFrameTime;
  uint iteration_counter;
  SdlGl vis;
};
