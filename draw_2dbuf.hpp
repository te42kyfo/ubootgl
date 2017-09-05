#pragma once
#include <glm/glm.hpp>
#include "texture.hpp"

namespace Draw2DBuf {
void init();

void draw_mag(float* buf_vx, float* buf_vy, int nx, int ny, glm::mat4 PVM);
void draw_scalar(float* buf_scalar, int nx, int ny, glm::mat4 PVM);
void draw_flag(Texture fill_tex, float* buf_flag, int nx, int ny,
               glm::mat4 PVM);
}
