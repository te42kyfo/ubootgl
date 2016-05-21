#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include "dtime.hpp"
#include "ubootgl_app.hpp"

using namespace std;

int main(int argc, char *argv[]) {
  UbootGlApp app;

  SDL_Event e;
  bool quit = false;

  while (!quit) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }

      if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_q) {
          quit = true;
        }
        app.handle_keys(e);
      }

      if (e.type == SDL_WINDOWEVENT) {
        if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
          app.vis.setViewport(e.window.data1, e.window.data2);
        }
      }
      if (e.type == SDL_MOUSEBUTTONDOWN) {
        app.handle_mouse(e);
      }
    }

    app.loop();
  }

  SDL_Quit();
}
