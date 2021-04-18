//#include "draw_2dbuf.hpp"
#include "db2dgrid.hpp"
#include "gl_error.hpp"
#include "load_shader.hpp"
#include "texture.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

using namespace std;

namespace Draw2DBuf {

GLuint mag_shader;
Shader flag_shader;
Shader scalar_shader;
GLuint vao, vbo, tbo, tex_id;

GLint mag_shader_tex_uloc, mag_shader_TM_uloc, mag_shader_bounds_uloc;
GLint flag_shader_mask_tex_uloc, flag_shader_fill_tex_uloc, flag_shader_TM_uloc;

float tex_min = 0;
float tex_max = 0;
float spread = 0.0;
float tex_median = 0;

void init() {
  // Magnitude Shader and uloc loading
  mag_shader = loadShader("./scale_translate2D.vert", "./mag_tex.frag",
                          {{0, "in_Position"}});
  mag_shader_tex_uloc = glGetUniformLocation(mag_shader, "tex");
  mag_shader_bounds_uloc = glGetUniformLocation(mag_shader, "bounds");
  mag_shader_TM_uloc = glGetUniformLocation(mag_shader, "TM");

  // Flag Field Shader and uloc loading
  flag_shader = Shader(loadShader("./scale_translate2D.vert", "./flag_tex.frag",
                           {{0, "in_Position"}}));
  scalar_shader = Shader(loadShader("./scale_translate2D.vert",
                                    "./color_tex.frag", {{0, "in_Position"}}));

  // Quad Geometry
  GL_CALL(glGenVertexArrays(1, &vao));
  GL_CALL(glGenBuffers(1, &vbo));

  float vertex_data[4][4] = {{0.0f, 0.0f, 0.0f, 1.0f},
                             {1.0f, 0.0f, 0.0f, 1.0f},
                             {0.0f, 1.0f, 0.0f, 1.0f},
                             {1.0f, 1.0f, 0.0f, 1.0f}};

  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), vertex_data,
                       GL_STATIC_DRAW));
}

glm::mat4 transformationMatrix(int tex_width, int tex_height, float pwidth,
                               glm::mat4 PVM) {
  // Model
  glm::mat4 TM = glm::scale(
      PVM, glm::vec3(pwidth, pwidth * (float)tex_height / tex_width, 1.0f));

  return TM;
}

void draw_mag(GLuint tex_id, int nx, int ny, glm::mat4 PVM, float pwidth) {

  float vmin = 0;
  float vmax = 1.0;
  GL_CALL(glUseProgram(mag_shader));
  GL_CALL(glUniform1i(mag_shader_tex_uloc, 0));

  glm::mat4 TM = transformationMatrix(nx, ny, pwidth, PVM);
  GL_CALL(
      glUniformMatrix4fv(mag_shader_TM_uloc, 1, GL_FALSE, glm::value_ptr(TM)));

  GL_CALL(glUniform2f(mag_shader_bounds_uloc, vmin, vmax));

  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_id));

  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR_MIPMAP_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  // Draw Quad with texture
  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  GL_CALL(glUseProgram(0));
}

void draw_flag(Texture fill_tex, GLuint flag_tex_id, int nx, int ny,
               glm::mat4 PVM, float pwidth, float offset) {
  GL_CALL(glUseProgram(flag_shader.id));
  GL_CALL(glUniform1i(flag_shader.uloc("mask_tex"), 0));
  GL_CALL(glUniform1i(flag_shader.uloc("fill_tex"), 1));
  GL_CALL(glUniform1f(flag_shader.uloc("offset"), offset));

  glm::mat4 TM = transformationMatrix(nx, ny, pwidth, PVM);

  GL_CALL(
      glUniformMatrix4fv(flag_shader_TM_uloc, 1, GL_FALSE, glm::value_ptr(TM)));

  GL_CALL(glActiveTexture(GL_TEXTURE1));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, fill_tex.tex_id));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, flag_tex_id));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR_MIPMAP_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  // Draw Quad with texture
  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  GL_CALL(glUseProgram(0));
}

void draw(float const *texture_buffer, int tex_width, int tex_height) {
  // Upload Texture

  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glGenTextures(1, &tex_id));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_id));

  int mip_levels = min(4, (int)min(floor(log2(tex_width)), log2(tex_height)));

  GL_CALL(glTexStorage2D(GL_TEXTURE_2D, mip_levels, GL_R32F, tex_width,
                         tex_height));
  GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_RED,
                          GL_FLOAT, texture_buffer));
  GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR_MIPMAP_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 0.0));

  // Draw Quad with texture
  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  GL_CALL(glDeleteTextures(1, &tex_id));
}

template <typename T>
void draw_buf(const T &grid, glm::mat4 PVM, float pwidth) {

  GL_CALL(glUseProgram(scalar_shader.id));
  GL_CALL(glUniform1i(scalar_shader.uloc("tex"), 0));

  glm::mat4 TM = transformationMatrix(grid.width, grid.height, pwidth, PVM);

  GL_CALL(glUniformMatrix4fv(scalar_shader.uloc("TM"), 1, GL_FALSE,
                             glm::value_ptr(TM)));

  draw(grid.data(), grid.width, grid.height);

  GL_CALL(glUseProgram(0));
}

template void draw_buf<Single2DGrid>(const Single2DGrid &grid, glm::mat4 PVM,
                                     float pwidth);
template void draw_buf<DoubleBuffered2DGrid>(const DoubleBuffered2DGrid &grid,
                                             glm::mat4 PVM, float pwidth);

} // namespace Draw2DBuf
