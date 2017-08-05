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
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSdlGL3_ProcessEvent(&event);
      if (event.type == SDL_QUIT) done = true;
    }

    app.loop();
    ImGui_ImplSdlGL3_NewFrame(app.vis.window);

    ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Window");
    ImGui::Text("Hello Amelie");
    ImGui::End();

    glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x,
               (int)ImGui::GetIO().DisplaySize.y);
    app.draw();

    ImGui::Render();
    SDL_GL_SwapWindow(app.vis.window);
  }

  // Cleanup
  ImGui_ImplSdlGL3_Shutdown();

  return 0;
}
