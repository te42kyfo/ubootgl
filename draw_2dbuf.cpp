#include "draw_2dbuf.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include "draw_text.hpp"
#include "gl_error.hpp"
#include "load_shader.hpp"
using namespace std;

namespace Draw2DBuf {

GLuint scalar_tex_shader, line_shader;
GLuint vao, vbo, tbo, tex_id, color_bar_tex_id;
int color_bar_tex_width = 20;
int color_bar_tex_height = 255;
vector<unsigned int> line_indices(4 * 2);
GLint scalar_tex_shader_tex_uloc, scalar_tex_shader_aspect_ratio_uloc,
    scalar_tex_shader_origin_uloc, scalar_tex_shader_bounds_uloc,
    line_shader_aspect_ratio_uloc, line_shader_origin_uloc;
float tex_min = 0;
float tex_max = 0;
float spread = 0.0;
float tex_median = 0;

void init() {
  scalar_tex_shader = loadShader("./scale_translate2D.vert",
                                 "./scalar_tex.frag", {{0, "in_Position"}});
  scalar_tex_shader_tex_uloc = glGetUniformLocation(scalar_tex_shader, "tex");
  scalar_tex_shader_bounds_uloc =
      glGetUniformLocation(scalar_tex_shader, "bounds");
  scalar_tex_shader_aspect_ratio_uloc =
      glGetUniformLocation(scalar_tex_shader, "aspect_ratio");
  scalar_tex_shader_origin_uloc =
      glGetUniformLocation(scalar_tex_shader, "origin");
  line_shader = loadShader("./scale_translate2D.vert", "./white.frag",
                           {{0, "in_Position"}});
  line_shader_aspect_ratio_uloc =
      glGetUniformLocation(line_shader, "aspect_ratio");
  line_shader_origin_uloc = glGetUniformLocation(line_shader, "origin");

  GL_CALL(glGenVertexArrays(1, &vao));
  GL_CALL(glGenBuffers(1, &vbo));

  float vertex_data[4][4] = {{-1.0f, -1.0f, 0.0f, 1.0f},
                             {1.0f, -1.0f, 0.0f, 1.0f},
                             {-1.0f, 1.0f, 0.0f, 1.0f},
                             {1.0f, 1.0f, 0.0f, 1.0f}};
  line_indices = {0, 1, 1, 3, 3, 2, 2, 0};

  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), vertex_data,
                       GL_STATIC_DRAW));

  // color bar texture
  vector<float> color_bar_tex_data(color_bar_tex_width * color_bar_tex_height);
  for (int y = 0; y < color_bar_tex_height; y++) {
    for (int x = 0; x < color_bar_tex_width; x++) {
      color_bar_tex_data[y * color_bar_tex_width + x] =
          (float)y / (color_bar_tex_height - 1);
    }
  }
  GL_CALL(glGenTextures(1, &color_bar_tex_id));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, color_bar_tex_id));
  GL_CALL(glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, color_bar_tex_width,
                         color_bar_tex_height));
  GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, color_bar_tex_width,
                          color_bar_tex_height, GL_RED, GL_FLOAT,
                          color_bar_tex_data.data()));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
}

void pegToOne(float& xOut, float& yOut, float xIn, float yIn) {
  xOut = xIn / max(xIn, yIn);
  yOut = yIn / max(xIn, yIn);
}

void draw(float* texture_buffer, int tex_width, int tex_height,
          int screen_width, int screen_height, float scale, float vmin,
          float vmax) {
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

  GL_CALL(glLineWidth(2));

  float ratio_x = 0;
  float ratio_y = 0;
  pegToOne(ratio_x, ratio_y,
           (float)screen_height / screen_width * tex_width / tex_height, 1.0f);

  // Upload Texture
  GL_CALL(glGenTextures(1, &tex_id));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_id));
  GL_CALL(glTexStorage2D(GL_TEXTURE_2D, 4, GL_R32F, tex_width, tex_height));
  GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_RED,
                          GL_FLOAT, texture_buffer));
  GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR_MIPMAP_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

  // Draw Quad with texture
  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glUseProgram(scalar_tex_shader));
  GL_CALL(glUniform1i(scalar_tex_shader_tex_uloc, 0));
  GL_CALL(glUniform2f(scalar_tex_shader_origin_uloc, 0.0, 0.0));
  GL_CALL(glUniform2f(scalar_tex_shader_aspect_ratio_uloc, scale * ratio_x,
                      scale * ratio_y));
  GL_CALL(glUniform2f(scalar_tex_shader_bounds_uloc, vmin, vmax));

  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  // Draw Quad with color bar
  GL_CALL(glBindTexture(GL_TEXTURE_2D, color_bar_tex_id));
  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glUniform1i(scalar_tex_shader_tex_uloc, 0));
  GL_CALL(glUniform2f(scalar_tex_shader_aspect_ratio_uloc, 0.02, 1.0));
  GL_CALL(glUniform2f(scalar_tex_shader_bounds_uloc, 0.0, 1.0));
  GL_CALL(glUniform2f(scalar_tex_shader_origin_uloc, 0.76, 0.0));
  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  DrawText::draw(to_string(vmin), 0.8, -0.9, 0.03, screen_width, screen_height);
  DrawText::draw(to_string(vmax), 0.8, 0.9, 0.03, screen_width, screen_height);

  // Draw Border
  /*  GL_CALL(glUseProgram(line_shader));
  GL_CALL(glUniform2f(line_shader_aspect_ratio_uloc, scale * ratio_x,
                      scale * ratio_y));
  GL_CALL(glUniform2f(line_shader_origin_uloc, 0.0, 0.0));
  GL_CALL(glDrawElements(GL_LINES, line_indices.size(), GL_UNSIGNED_INT,
  line_indices.data()));*/
  GL_CALL(glDeleteTextures(1, &tex_id));
}

void draw_mag(float* buf_vx, float* buf_vy, int nx, int ny, int screen_width,
              int screen_height, float scale) {
  std::vector<float> V(nx * ny);
  std::vector<float> vt(nx * ny);

  for (int y = 0; y < ny; y++) {
    for (int x = 0; x < nx; x++) {
      int ind = y * nx + x;
      V[ind] = sqrt(std::max(
          0.0f, buf_vx[ind] * buf_vx[ind] + buf_vy[ind] * buf_vy[ind]));
      vt[ind] = V[ind];
    }
  }

  auto upper_bound = vt.begin() + vt.size() * 0.99;
  nth_element(vt.begin(), upper_bound, vt.end());

  Draw2DBuf::draw(V.data(), nx, ny, screen_width, screen_height, scale, 0,
                  *upper_bound);
}
}
