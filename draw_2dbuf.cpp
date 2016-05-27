#include "draw_2dbuf.hpp"
#include <GL/glew.h>
#include <cmath>
#include <iostream>
#include "gl_error.hpp"
#include "load_shader.hpp"

using namespace std;

namespace Draw2DBuf {

GLuint scalar_tex_shader, line_shader;
GLuint vao, vbo, tbo, tex_id;
vector<unsigned int> line_indices(4 * 2);
GLint scalar_tex_shader_tex_uloc, scalar_tex_shader_aspect_ratio_uloc,
    line_shader_aspect_ratio_uloc, scalar_tex_shader_bounds_uloc;

void init() {

  scalar_tex_shader =
      loadShader("./scale2D.vert", "./scalar_tex.frag", {{0, "in_Position"}});
  scalar_tex_shader_tex_uloc = glGetUniformLocation(scalar_tex_shader, "tex");
  scalar_tex_shader_bounds_uloc =
      glGetUniformLocation(scalar_tex_shader, "bounds");
  scalar_tex_shader_aspect_ratio_uloc =
      glGetUniformLocation(scalar_tex_shader, "aspect_ratio");
  line_shader =
      loadShader("./scale2D.vert", "./white.frag", {{0, "in_Position"}});
  line_shader_aspect_ratio_uloc =
      glGetUniformLocation(line_shader, "aspect_ratio");

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

void pegToOne(float& xOut, float& yOut, float xIn, float yIn) {
  xOut = xIn / max(xIn, yIn);
  yOut = yIn / max(xIn, yIn);
}

void draw(float* texture_buffer, int tex_width, int tex_height, int screen_width,
          int screen_height, float scale) {
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

  GL_CALL(glLineWidth(2));

  float ratio_x = 0;
  float ratio_y = 0;

  float tex_min = texture_buffer[0];
  float tex_max = texture_buffer[0];
  for (int i = 0; i < tex_width * tex_height; i++) {
    tex_min = min(tex_min, texture_buffer[i]);
    tex_max = max(tex_max, texture_buffer[i]);
  }

  pegToOne(ratio_x, ratio_y,
           (float)screen_height / screen_width * tex_width / tex_height, 1.0f);

  // Upload Texture
  GL_CALL(glGenTextures(1, &tex_id));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_id));
  GL_CALL(glTexStorage2D(GL_TEXTURE_2D, 8, GL_R32F, tex_width, tex_height));
  GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width, tex_height, GL_RED,
                          GL_FLOAT, texture_buffer));
  GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR_MIPMAP_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  // Draw Quad with texture
  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glUseProgram(scalar_tex_shader));
  GL_CALL(glUniform1i(scalar_tex_shader_tex_uloc, 0));
  GL_CALL(glUniform2f(scalar_tex_shader_aspect_ratio_uloc, scale * ratio_x,
                      scale * ratio_y));
  GL_CALL(glUniform2f(scalar_tex_shader_bounds_uloc, tex_min, tex_max));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
  GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

  // Draw Border
  GL_CALL(glUseProgram(line_shader));
  GL_CALL(glUniform2f(line_shader_aspect_ratio_uloc, scale * ratio_x,
                      scale * ratio_y));
  GL_CALL(glDrawElements(GL_LINES, line_indices.size(), GL_UNSIGNED_INT,
                         line_indices.data()));
  GL_CALL(glDeleteTextures(1, &tex_id));
}
}
