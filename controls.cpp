#include "controls.hpp"
#include <sstream>
using namespace std;

std::map<std::pair<int, CONTROLS>, SDL_Keycode> key_map = {
    {{0, CONTROLS::THRUST_FORWARD}, SDLK_UP},
    {{0, CONTROLS::THRUST_BACKWARD}, SDLK_DOWN},
    {{0, CONTROLS::TURN_CLOCKWISE}, SDLK_RIGHT},
    {{0, CONTROLS::TURN_COUNTERCLOCKWISE}, SDLK_LEFT},
    {{0, CONTROLS::LAUNCH_TORPEDO}, SDLK_MINUS},
    {{1, CONTROLS::THRUST_FORWARD}, SDLK_w},
    {{1, CONTROLS::THRUST_BACKWARD}, SDLK_s},
    {{1, CONTROLS::TURN_CLOCKWISE}, SDLK_d},
    {{1, CONTROLS::TURN_COUNTERCLOCKWISE}, SDLK_a},
    {{1, CONTROLS::LAUNCH_TORPEDO}, SDLK_TAB},
    {{2, CONTROLS::THRUST_FORWARD}, SDLK_t},
    {{2, CONTROLS::THRUST_BACKWARD}, SDLK_g},
    {{2, CONTROLS::TURN_CLOCKWISE}, SDLK_h},
    {{2, CONTROLS::TURN_COUNTERCLOCKWISE}, SDLK_f},
    {{2, CONTROLS::LAUNCH_TORPEDO}, SDLK_v},
    {{3, CONTROLS::THRUST_FORWARD}, SDLK_i},
    {{3, CONTROLS::THRUST_BACKWARD}, SDLK_k},
    {{3, CONTROLS::TURN_CLOCKWISE}, SDLK_l},
    {{3, CONTROLS::TURN_COUNTERCLOCKWISE}, SDLK_j},
    {{3, CONTROLS::LAUNCH_TORPEDO}, SDLK_n}};

string getControlString(int keySet) {
  stringstream controlString;
  controlString
      << SDL_GetKeyName(key_map[{keySet, CONTROLS::THRUST_FORWARD}]) << " "
      << SDL_GetKeyName(key_map[{keySet, CONTROLS::TURN_COUNTERCLOCKWISE}])
      << " " << SDL_GetKeyName(key_map[{keySet, CONTROLS::THRUST_BACKWARD}])
      << " " << SDL_GetKeyName(key_map[{keySet, CONTROLS::TURN_CLOCKWISE}])
      << " " << SDL_GetKeyName(key_map[{keySet, CONTROLS::LAUNCH_TORPEDO}]);
  return controlString.str();
}
