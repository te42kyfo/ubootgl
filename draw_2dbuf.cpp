#include "draw_2dbuf.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include "draw_text.hpp"
#include "gl_error.hpp"
#include "load_shader.hpp"
#include "texture.hpp"
using namespace std;

namespace Draw2DBuf {

GLuint mag_shader;
GLuint flag_shader;
GLuint vao, vbo, tbo, tex_id;

vector<unsigned int> line_indices(4 * 2);
GLint mag_shader_tex_uloc, mag_shader_aspect_ratio_uloc, mag_shader_origin_uloc,
    mag_shader_bounds_uloc;
GLint flag_shader_mask_tex_uloc, flag_shader_fill_tex_uloc,
    flag_shader_aspect_ratio_uloc, flag_shader_origin_uloc;

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
  mag_shader_aspect_ratio_uloc =
      glGetUniformLocation(mag_shader, "aspect_ratio");
  mag_shader_origin_uloc = glGetUniformLocation(mag_shader, "origin");

  // Flag Field Shader and uloc loading
  flag_shader = loadShader("./scale_translate2D.vert", "./flag_tex.frag",
                           {{0, "in_Position"}});
  flag_shader_mask_tex_uloc = glGetUniformLocation(flag_shader, "mask_tex");
  flag_shader_fill_tex_uloc = glGetUniformLocation(flag_shader, "fill_tex");
  flag_shader_aspect_ratio_uloc =
      glGetUniformLocation(flag_shader, "aspect_ratio");
  flag_shader_origin_uloc = glGetUniformLocation(flag_shader, "origin");

  // Quad Geometry
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
}

void draw(float* texture_buffer, int tex_width, int tex_height,
          int screen_width, int screen_height, float scale) {
  // Upload Texture

  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glGenTextures(1, &tex_id));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_id));

  int mip_levels = min(4, (int)min(floor(log2(tex_width)), log2(tex_height)));

  GL_CALL(glTexStorage2D(GL_TEXTURE_2D, mip_levels, GL_R32F, tex_width, tex_height));
  GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_RED,
                          GL_FLOAT, texture_buffer));
  GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR_MIPMAP_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

  // Draw Quad with texture
  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  GL_CALL(glDeleteTextures(1, &tex_id));
}

struct vec2 {
  float x, y;
};

vec2 pegToOne(float xIn, float yIn) {
  return {xIn / max(xIn, yIn), yIn / max(xIn, yIn)};
}

vec2 calculateScaleFactors(int render_width, int render_height, int tex_width,
                           int tex_height) {
  return pegToOne((float)render_height / render_width * tex_width / tex_height,
                  1.0f);
}

void draw_scalar(float* buf_scalar, int nx, int ny, int screen_width,
                 int screen_height, float scale) {
  vector<float> vt(buf_scalar, buf_scalar + nx*ny);
  auto upper_bound = vt.begin() + vt.size() - 1;
  nth_element(vt.begin(), upper_bound, vt.end());
  float vmax = *upper_bound;
  auto lower_bound = vt.begin() + vt.size() *0;
  nth_element(vt.begin(), lower_bound, vt.end());
  float vmin = *lower_bound;

  cout << "Draw_scalar bounds: " << vmin << " " << vmax << "\n";

  GL_CALL(glUseProgram(mag_shader));
  GL_CALL(glUniform1i(mag_shader_tex_uloc, 0));
  GL_CALL(glUniform2f(mag_shader_origin_uloc, 0.0, 0.0));

  vec2 ratios = calculateScaleFactors(screen_width, screen_height, nx, ny);
  GL_CALL(glUniform2f(mag_shader_aspect_ratio_uloc, scale * ratios.x,
                      scale * ratios.y));
  GL_CALL(glUniform2f(mag_shader_bounds_uloc, vmin, vmax));

  Draw2DBuf::draw(buf_scalar, nx, ny, screen_width, screen_height, scale);
  GL_CALL(glUseProgram(0));
}

void draw_mag(float* buf_vx, float* buf_vy, int nx, int ny, int screen_width,
              int screen_height, float scale) {
  std::vector<float> V(nx * ny);
  std::vector<float> vt(nx * ny);

  for (int y = 0; y < ny; y++) {
    for (int x = 0; x < nx; x++) {
      int ind = y * nx + x;

      // + small constant avoids numerical problems with fast sqrts
      V[ind] = sqrt(buf_vx[ind] * buf_vx[ind] + buf_vy[ind] * buf_vy[ind] +
                    0.00001f);
      vt[ind] = V[ind];
    }
  }

  auto upper_bound = vt.begin() + vt.size() * 0.99;
  nth_element(vt.begin(), upper_bound, vt.end());

  float vmin = 0;
  float vmax = *upper_bound;
  GL_CALL(glUseProgram(mag_shader));
  GL_CALL(glUniform1i(mag_shader_tex_uloc, 0));
  GL_CALL(glUniform2f(mag_shader_origin_uloc, 0.0, 0.0));

  vec2 ratios = calculateScaleFactors(screen_width, screen_height, nx, ny);
  GL_CALL(glUniform2f(mag_shader_aspect_ratio_uloc, scale * ratios.x,
                      scale * ratios.y));
  GL_CALL(glUniform2f(mag_shader_bounds_uloc, vmin, vmax));

  Draw2DBuf::draw(V.data(), nx, ny, screen_width, screen_height, scale);
  GL_CALL(glUseProgram(0));
}

void draw_flag(Texture fill_tex, float* buf_flag, int nx, int ny,
               int screen_width, int screen_height, float scale) {
  GL_CALL(glUseProgram(flag_shader));
  GL_CALL(glUniform1i(flag_shader_mask_tex_uloc, 0));
  GL_CALL(glUniform1i(flag_shader_fill_tex_uloc, 1));
  GL_CALL(glUniform2f(flag_shader_origin_uloc, 0.0, 0.0));

  GL_CALL(glActiveTexture(GL_TEXTURE1));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, fill_tex.tex_id));

  //  GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 20.0));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  vec2 ratios = calculateScaleFactors(screen_width, screen_height, nx, ny);
  GL_CALL(glUniform2f(flag_shader_aspect_ratio_uloc, scale * ratios.x,
                      scale * ratios.y));

  Draw2DBuf::draw(buf_flag, nx, ny, screen_width, screen_height, scale);
  GL_CALL(glUseProgram(0));
}
}
