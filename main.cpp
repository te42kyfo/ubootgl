#include "dtime.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"
#include "ubootgl_app.hpp"
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <omp.h>
#include <stdio.h>

using namespace std;

void *simulationLoop(void *arg);

int main(int, char **) {
  UbootGlApp app;

  pthread_t threadId;
  pthread_create(&threadId, NULL, simulationLoop,
                 reinterpret_cast<void *>(&app));

  ImGui_ImplSdlGL3_Init(app.vis.window);

  ImGuiIO &io = ImGui::GetIO();
  ImFontConfig config;
  config.GlyphExtraSpacing.x = 1.0f; // Increase spacing between characters
  io.Fonts->AddFontFromFileTTF("resources/DroidSans.ttf", 17, &config);

  // Main loop
  bool done = false;
  SDL_JoystickOpen(0);
  SDL_JoystickOpen(1);

  double lastFrameTime = dtime();

  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSdlGL3_ProcessEvent(&event);
      switch (event.type) {
      case SDL_QUIT:
        done = true;
        break;
      case SDL_KEYDOWN:
      case SDL_KEYUP:
        app.handleKey(event.key);
        break;
      case SDL_JOYAXISMOTION:
        app.handleJoyAxis(event.jaxis);
        break;

      case SDL_JOYBUTTONDOWN:
      case SDL_JOYBUTTONUP:
        app.handleJoyButton(event.jbutton);
        break;
      }
    }

    ImGui_ImplSdlGL3_NewFrame(app.vis.window);
    app.draw();
    ImGui::Render();

    glFlush();
    app.loop();
    SDL_GL_SwapWindow(app.vis.window);

    double newFrameTime = dtime();
    app.frameTimes.add(1000.0f * (newFrameTime - lastFrameTime));
    lastFrameTime = newFrameTime;
  }

  // Cleanup
  ImGui_ImplSdlGL3_Shutdown();

  return 0;
}

void *simulationLoop(void *arg) {

  UbootGlApp *app = reinterpret_cast<UbootGlApp *>(arg);

  int threadCount = 0;
#pragma omp parallel
  { threadCount = omp_get_num_threads(); }
  int simulationThreads = max(1, threadCount / 2 - 1);
  cout << simulationThreads << "/" << threadCount << " threads\n";
  omp_set_num_threads(simulationThreads);
  double tprev = dtime();
  double smoothedSimTime = 0.0;
  while (true) {

    app->simTimeStep = 0.1f * min(0.2, smoothedSimTime);
    app->sim.step(app->simTimeStep);

    double tnow = dtime();
    double dt = tnow - tprev;
    app->simTimes.add(dt * 1000.f);
    tprev = tnow;
    smoothedSimTime = smoothedSimTime * 0.95 + 0.05 * dt;

    static int frameCounter = 0;
    frameCounter ++;
    if (frameCounter % 10 == 0) {
      app->shiftMap();
    }
  }
  return nullptr;
}
