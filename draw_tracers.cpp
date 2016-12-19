#include "draw_tracers.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include "gl_error.hpp"
#include "load_shader.hpp"

using namespace std;

namespace DrawTracers {

GLuint tracer_shader;
GLuint vao, vbo_vertices, vbo_alphas;
GLint tracer_shader_aspect_ratio_uloc, tracer_shader_origin_uloc;

struct vec2 {
  float x, y;
};
struct vec4 {
  float x, y, z, w;
};

vector<vec4> tracers;
vector<float> alphas;
default_random_engine gen;
uniform_real_distribution<float> dis(0.0, 1.0);

void init() {
  tracer_shader = loadShader("./scale_translate2D.vert", "./color.frag",
                             {{0, "in_Position"}, {1, "in_Alpha"}});
  tracer_shader_aspect_ratio_uloc =
      glGetUniformLocation(tracer_shader, "aspect_ratio");
  tracer_shader_origin_uloc = glGetUniformLocation(tracer_shader, "origin");
  GL_CALL(glGenVertexArrays(1, &vao));
};

void drawTracers(const vector<vec4>& tracers, const vector<float>& alphas,
                 float ratio_x, float ratio_y, float x_offset, float y_offset) {
  GL_CALL(glEnable(GL_BLEND));
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE));

  GL_CALL(glBindVertexArray(vao));

  GL_CALL(glGenBuffers(1, &vbo_alphas));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_alphas));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, alphas.size() * sizeof(GLfloat),
                       alphas.data(), GL_DYNAMIC_DRAW));
  GL_CALL(glEnableVertexAttribArray(1));
  GL_CALL(glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glGenBuffers(1, &vbo_vertices));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, tracers.size() * sizeof(vec4),
                       tracers.data(), GL_DYNAMIC_DRAW));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glUseProgram(tracer_shader));
  GL_CALL(glUniform2f(tracer_shader_aspect_ratio_uloc, ratio_x, ratio_y));
  GL_CALL(glUniform2f(tracer_shader_origin_uloc, x_offset, y_offset));

  GL_CALL(glDrawArrays(GL_POINTS, 0, tracers.size()));

  GL_CALL(glDeleteBuffers(1, &vbo_vertices));
  GL_CALL(glDeleteBuffers(1, &vbo_alphas));
}

float bilinearSample(float x, float y, float* v, int nx, int ny) {
  int ix = floor(x - 0.5f);
  int iy = floor(y - 0.5f);
  if (ix < 0 || ix >= nx - 1 || iy < 0 || iy >= ny - 1) return 0.0f;

  float s = x - 0.5f - ix;
  float t = y - 0.5f - iy;
  float as = 1.0f - s;
  float at = 1.0f - t;
  int idx1 = iy * nx + ix;
  int idx2 = iy * nx + ix + 1;
  int idx3 = (iy + 1) * nx + ix;
  int idx4 = (iy + 1) * nx + ix + 1;

  return at * (as * v[idx1] + s * v[idx2]) + t * (as * v[idx3] + s * v[idx4]);
}

void pegToOne(float& xOut, float& yOut, float xIn, float yIn) {
  xOut = xIn / max(xIn, yIn);
  yOut = yIn / max(xIn, yIn);
}
void draw(float* vx, float* vy, int nx, int ny, int screen_width,
          int screen_height, float scale, float dt) {
  float screen_ratio_x = 0;
  float screen_ratio_y = 0;
  pegToOne(screen_ratio_x, screen_ratio_y,
           (float)screen_height / screen_width * nx / ny, 1.0f);
  float ratio_x = 2.0 * screen_ratio_x / nx;
  float ratio_y = 2.0 * screen_ratio_y / ny;

  for (int i = 0; i < 200; i++) {
    tracers.push_back({dis(gen) * nx * 0.01, dis(gen) * ny, 0.0, 1.0});
    auto& tracer = tracers.back();
    alphas.push_back(1.0);
  }

#pragma omp parallel for
  for (size_t i = 0; i < tracers.size(); i++) {
    auto& tracer = tracers[i];
    float dx = bilinearSample(tracer.x, tracer.y, vx, nx, ny);
    float dy = bilinearSample(tracer.x, tracer.y, vy, nx, ny);

    tracer.x += dx * dt;
    tracer.y += dy * dt;
    alphas[i] -= 0.001;
    if (tracer.x > nx - 1 || tracer.x < 1 || tracer.y > ny - 1 ||
        tracer.y < 1 || alphas[i] < -10.001) {
      tracer = {dis(gen) * nx * 0.01, dis(gen) * ny, 0.0, 1.0};
      alphas[i] = 1.0;
    }
  }

  drawTracers(tracers, alphas, ratio_x * scale, ratio_y * scale,
              -1.0f * screen_ratio_x * scale, -1.0f * screen_ratio_y * scale);
}
}
