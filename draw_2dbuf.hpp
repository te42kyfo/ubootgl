#include <glm/vec2.hpp>
#include "texture.hpp"

namespace Draw2DBuf {
void init();
void draw(float* buf, int nx, int ny, int screen_width, int screen_height);
void draw_mag(float* buf_vx, float* buf_vy, int nx, int ny, int screen_width,
              int screen_height, float scale, glm::vec2 translate);
void draw_scalar(float* buf_scalar, int nx, int ny, int screen_width,
                 int screen_height, float scale, glm::vec2 translate);
void draw_flag(Texture fill_tex, float* buf_flag, int nx, int ny,
               int screen_width, int screen_height, float scale,
               glm::vec2 translate);
}
