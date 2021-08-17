#pragma once
#include "texture.hpp"
#include <glm/glm.hpp>

namespace Draw2DBuf {
void init();

template<typename T>
void draw_buf(const T &grid, glm::mat4 PVM, float pwidth);


void draw_mag_flag(Texture fill_tex, GLuint flag_tex_id, GLuint mag_tex_id, int nx, int ny,
                   glm::mat4 PVM, float pwidth, float offset = 0.0);

void draw_mag(GLuint tex_id, int nx, int ny, glm::mat4 PVM, float pwidth);
void draw_flag(GLuint flag_tex_id, int nx, int ny,
               glm::mat4 PVM, float pwidth);
} // namespace Draw2DBuf
