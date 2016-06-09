#pragma once

#include <iostream>
#include <vector>
#include "draw_2dbuf.hpp"
#include "draw_streamlines.hpp"
#include "draw_text.hpp"
#include "dtime.hpp"
#include "gl_error.hpp"
#include "sdl_gl.hpp"
#include "simulation.hpp"

class UbootGlApp {
 public:
  UbootGlApp() : sim(1.0, 6000.0f, 129, 129) {
    vis.initDisplay(1);
    vis.setViewport(800, 600);
    Draw2DBuf::init();
    DrawText::init();
    DrawStreamlines::init();
    last_frame_time = dtime();
    render_time = 0;
    simulation_time = 0;
    simulation_steps = 0;
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
      simulation_steps++;
    }
    double t2 = dtime();
    simulation_time += t2 - t1;
    draw();
    double t3 = dtime();
    render_time += t3 - t2;

    iteration_counter++;
    double this_frame_time = dtime();
    if (this_frame_time - last_frame_time > 0.5) {
      frame_rate = iteration_counter / (this_frame_time - last_frame_time);
      std::cout << std::setw(3) << std::fixed << std::setprecision(1)
                << simulation_time / iteration_counter * 1000.0 << "("
                << simulation_time / simulation_steps * 1000.0 << ") "
                << std::setw(3) << render_time / iteration_counter * 1000.0
                << " "
                << (this_frame_time - last_frame_time) / iteration_counter *
                       1000.0
                << "\n";
      iteration_counter = 0;
      last_frame_time = this_frame_time;
      render_time = 0;
      simulation_time = 0;
      simulation_steps = 0;
    }
  }
  void draw() {
    SDL_GL_MakeCurrent(vis.windows[0], vis.gl_context);
    GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    Draw2DBuf::draw_mag(sim.getVX(), sim.getVY(), sim.width, sim.height,
                        vis.pixel_width, vis.pixel_height, scale);
    DrawStreamlines::draw(sim.getVX(), sim.getVY(), sim.width, sim.height,
                          vis.pixel_width, vis.pixel_height, scale);
    /*    DrawText::draw(std::to_string((int)frame_rate), -1, 0.9, 0.05,
          vis.pixel_width, vis.pixel_height);*/
    SDL_GL_SwapWindow(vis.windows[0]);
  }
  double frame_rate;
  float scale;
  double last_frame_time;
  double simulation_time;
  double render_time;
  int simulation_steps;
  uint iteration_counter;
  SdlGl vis;
  Simulation sim;
};
