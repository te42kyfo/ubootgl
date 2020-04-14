#pragma once
#include "texture.hpp"
#include <glm/glm.hpp>

namespace Draw2DBuf {
void init();

void draw_mag(GLuint tex_id, int nx, int ny, glm::mat4 PVM, float pwidth);
void draw_flag(Texture fill_tex, float *buf_flag, int nx, int ny, glm::mat4 PVM,
               float pwidth);
} // namespace Draw2DBuf
