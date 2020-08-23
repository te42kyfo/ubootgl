#include <cstdio>
#include <iomanip>
#include <iostream>

#include "ubootgl_app.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl_gl3.h"
#include <stdio.h>

using namespace std;

int main(int, char **) {
  UbootGlApp app;

  ImGui_ImplSdlGL3_Init(app.vis.window);

  ImGuiIO &io = ImGui::GetIO();
  ImFontConfig config;
  config.GlyphExtraSpacing.x = 1.0f; // Increase spacing between characters
  io.Fonts->AddFontFromFileTTF("resources/DroidSans.ttf", 17, &config);

  // Main loop
  bool done = false;
  SDL_JoystickOpen(0);
  SDL_JoystickOpen(1);
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
  }

  // Cleanup
  ImGui_ImplSdlGL3_Shutdown();

  return 0;
}
