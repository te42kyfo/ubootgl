#include <cstdio>
#include <iomanip>
#include <iostream>

#include "ubootgl_app.hpp"

#include <stdio.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"

using namespace std;

int main(int, char**) {
  UbootGlApp app;

  ImGui_ImplSdlGL3_Init(app.vis.window);

  ImGuiIO& io = ImGui::GetIO();
  ImFontConfig config;
  config.GlyphExtraSpacing.x = 1.0f; // Increase spacing between characters
  io.Fonts->AddFontFromFileTTF("resources/DroidSans.ttf", 17, &config);


  // Main loop
  bool done = false;

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
      }
    }

    ImGui_ImplSdlGL3_NewFrame(app.vis.window);
    app.draw();
    ImGui::Render();

    glFlush();
    app.loop();
    SDL_GL_SwapWindow(app.vis.window);
  }

  // Cleanup
  ImGui_ImplSdlGL3_Shutdown();

  return 0;
}
