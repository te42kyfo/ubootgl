#include <cstdio>
#include <iomanip>
#include <iostream>
#include "dtime.hpp"
#include "ubootgl_app.hpp"

#include <stdio.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"

using namespace std;

int main(int, char**) {
  UbootGlApp app;

  ImGui_ImplSdlGL3_Init(app.vis.window);

  // Main loop
  bool done = false;
  double lastFrameTime;
  double smoothedFrameRate = 0.0;
  while (!done) {
    double frameStart = dtime();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSdlGL3_ProcessEvent(&event);
      if (event.type == SDL_QUIT) done = true;
    }

    app.loop();
    ImGui_ImplSdlGL3_NewFrame(app.vis.window);

    bool p_open;
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(250, ImGui::GetIO().DisplaySize.y - 10));
    ImGui::Begin("Example: Fixed Overlay", &p_open,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_ShowBorders);

    ImGui::TextWrapped(app.sim.diag.str().c_str());
    ImGui::Separator();
    ImGui::TextWrapped("FPS: %.1f, %d sims/frame",  smoothedFrameRate,
                       app.iterationCounter);

    ImGui::End();

    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x,
               (int)ImGui::GetIO().DisplaySize.y);
    app.draw();

    ImGui::Render();
    SDL_GL_SwapWindow(app.vis.window);
    lastFrameTime = dtime() - frameStart;
    smoothedFrameRate = 0.95 * smoothedFrameRate + 0.05 / lastFrameTime;
  }

  // Cleanup
  ImGui_ImplSdlGL3_Shutdown();

  return 0;
}
