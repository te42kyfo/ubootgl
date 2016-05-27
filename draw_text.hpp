#include <string>

namespace DrawText {
void init();
void draw(std::string text, float x_pos, float y_pos, float size,
          int screen_width, int screen_height);
}
