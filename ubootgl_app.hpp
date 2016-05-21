#pragma once

#include <iostream>
#include <vector>
#include "dtime.hpp"
#include "sdl_gl.hpp"


class UbootGlApp {
 public:
  UbootGlApp() {
    vis.initDisplay(1);
    vis.setViewport(800, 600);
    lastFrameTime = dtime();
    iterationCounter = 0;
  }

  void handle_keys(const SDL_Event& e) {
    switch (e.key.keysym.sym) {}
  }
  void handle_mouse(const SDL_Event& e) {}

  void loop() {
    // double t1 = dtime();
    //    while (dtime() - t1 < 0.03) {
    // sim.step();
    //}

    draw();
    iterationCounter++;
    double thisFrameTime = dtime();
    if (thisFrameTime - lastFrameTime > 0.5) {
      std::cout << iterationCounter / (thisFrameTime - lastFrameTime) << "\n";
      iterationCounter = 0;
      lastFrameTime = thisFrameTime;
    }
  }
  void draw() {
    SDL_GL_MakeCurrent(vis.windows[0], vis.gl_context);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    
    SDL_GL_SwapWindow(vis.windows[0]);
  }

  double lastFrameTime;
  uint iterationCounter;
  SdlGl vis;
};
