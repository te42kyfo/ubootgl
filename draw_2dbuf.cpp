#include "draw_2dbuf.hpp"
#include <cmath>
#include <iostream>
#include "gl_error.hpp"
#include "load_shader.hpp"

using namespace std;

namespace Draw2DBuf {

GLuint slice_shader, line_shader;
GLuint vao, vbo, tbo;
vector<unsigned int> line_indices(4 * 2);
GLint slice_shader_tex_uloc, slice_shader_aspect_ratio_uloc,
    slice_shader_scaling_factor_uloc, line_shader_aspect_ratio_uloc;
float scaling_factor = 1.0f;

void init() {
  GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GL_CALL(glEnable(GL_BLEND));
  GL_CALL(glLineWidth(3));

  /*  slice_shader = loadShader("./draw_slice.vert", "./draw_slice.frag",
                            {{0, "in_Position"}});
  slice_shader_tex_uloc = glGetUniformLocation(slice_shader, "tex");
  slice_shader_aspect_ratio_uloc =
      glGetUniformLocation(slice_shader, "aspect_ratio");
  slice_shader_scaling_factor_uloc =
      glGetUniformLocation(slice_shader, "scaling_factor");
  */
  line_shader =
      loadShader("./scale2D.vert", "./white.frag", {{0, "in_Position"}});
  line_shader_aspect_ratio_uloc =
      glGetUniformLocation(line_shader, "aspect_ratio");

  GL_CALL(glGenVertexArrays(1, &vao));
  GL_CALL(glGenBuffers(1, &vbo));
  GL_CALL(glGenBuffers(1, &tbo));

  float vertex_data[4][4] = {{-1.0f, -1.0f, 0.0f, 1.0f},
                             {1.0f, -1.0f, 0.0f, 1.0f},
                             {-1.0f, 1.0f, 0.0f, 1.0f},
                             {1.0f, 1.0f, 0.0f, 1.0f}};
  line_indices = {0, 1, 1, 3, 3, 2, 2, 0};

  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), vertex_data,
                       GL_STATIC_DRAW));
}

void pegToOne(float& xOut, float& yOut, float xIn, float yIn) {
  xOut = xIn / max(xIn, yIn);
  yOut = yIn / max(xIn, yIn);
}

void draw(float* buf3d, size_t tex_width, size_t tex_height, int pixel_width,
          int pixel_height) {
  GL_CALL(glLineWidth(3));

  float ratio_x = 0;
  float ratio_y = 0;

  pegToOne(ratio_x, ratio_y,
           (float)pixel_height / pixel_width * tex_width / tex_height, 1.0f);

  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glEnableVertexAttribArray(0));

  // GLuint texId;
  // glGenTextures(1, &texId);
  // glBindTexture(GL_TEXTURE_2D, texId);
  // glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, nx, ny, 0, GL_RED, GL_FLOAT,
  // buf3d);
  // glGenerateMipmap(GL_TEXTURE_2D);

  // glActiveTexture(GL_TEXTURE0);

  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
  //                GL_LINEAR_MIPMAP_LINEAR);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  /*  glUseProgram(slice_shader);
  glUniform1i(slice_shader_tex_uloc, 0);
  glUniform2f(slice_shader_aspect_ratio_uloc,
              xratio * (1.0f - 5.0f / pixel_width),
              yratio * (1.0f - 5.0f / pixel_height));
  glUniform1f(slice_shader_scaling_factor_uloc, scaling_factor);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glDeleteTextures(1, &texId);
  */

  GL_CALL(glUseProgram(line_shader));
  GL_CALL(glUniform2f(line_shader_aspect_ratio_uloc, ratio_x, ratio_y));
  GL_CALL(glDrawElements(GL_LINES, line_indices.size(), GL_UNSIGNED_INT,
                         line_indices.data()));
}
}
