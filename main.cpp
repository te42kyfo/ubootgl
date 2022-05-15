#include "dtime.hpp"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "imgui/backends/imgui_impl_sdl.h"
#include "imgui/imgui.h"
#include "implot/implot.h"
#include "ubootgl_app.hpp"
#include <cstdio>
#include <functional>
#include <iomanip>
#include <iostream>
#include <omp.h>
#include <stdio.h>
#include <thread>

using namespace std;

void *simulationLoop(void *arg);

int main(int, char **) {
  UbootGlApp app;

  std::thread simulationThread(&UbootGlApp::sim_loop, &app);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImFontConfig config;
  config.GlyphExtraSpacing.x = 1.0f; // Increase spacing between characters
  io.Fonts->AddFontFromFileTTF("resources/DroidSans.ttf", 17, &config);
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(app.vis.window, app.vis.gl_context);
  ImGui_ImplOpenGL3_Init("#version 130");

  SDL_JoystickOpen(0);
  SDL_JoystickOpen(1);
  SDL_JoystickOpen(2);
  SDL_JoystickOpen(3);

  double lastFrameTime = dtime();

  // Main loop
  while (app.gameRunning) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      switch (event.type) {
      case SDL_QUIT:
        app.gameRunning = false;
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

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(app.vis.window);
    ImGui::NewFrame();
    app.draw();
    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glFlush();
    app.loop();
    SDL_GL_SwapWindow(app.vis.window);

    double newFrameTime = dtime();
    app.frameTimes.add(1000.0f * (newFrameTime - lastFrameTime));
    lastFrameTime = newFrameTime;
  }

  simulationThread.join();
  std::cout << "thread has joined\n";

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  return 0;
}
