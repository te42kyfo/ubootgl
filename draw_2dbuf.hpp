#pragma once
#include "texture.hpp"
#include <glm/glm.hpp>

namespace Draw2DBuf {
void init();

template<typename T>
void draw_buf(const T &grid, glm::mat4 PVM, float pwidth);


void draw_mag(GLuint tex_id, int nx, int ny, glm::mat4 PVM, float pwidth);
void draw_flag(Texture fill_tex, GLuint flag_tex_id, int nx, int ny,
               glm::mat4 PVM, float pwidth);
} // namespace Draw2DBuf
