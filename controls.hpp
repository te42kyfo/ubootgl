#pragma once
#include <SDL2/SDL.h>
#include <map>
#include <string>

enum class CONTROLS {
  THRUST_FORWARD,
  THRUST_BACKWARD,
  TURN_CLOCKWISE,
  TURN_COUNTERCLOCKWISE,
  LAUNCH_TORPEDO,
  JOY_ID
};

extern std::map<std::pair<int, CONTROLS>, SDL_Keycode> key_map;

std::string getControlString(int keySet);
