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

const int tracerCount = 10000;
const int tailCount = 100;

vector<vec2> tracers(tracerCount, {0, 0});
vector<vector<vec2>> vertices(tailCount, vector<vec2>(tracerCount * 2, {0, 0}));
vector<float> alphas(tracerCount, -10.0f);
vector<int> tailCounts(tracerCount);
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

void drawTracers(float ratio_x, float ratio_y, float x_offset, float y_offset) {
  vector<vec4> vBuf;
  vector<int> iBuf;
  vector<float> vAlphas;

  for (int t = 0; t < tracers.size(); t++) {
    for (int n = 0; n < tailCount; n++) {
      vBuf.push_back(
          {vertices[n][2 * t + 0].x, vertices[n][2 * t + 0].y, 0, 0});
      vBuf.push_back(
          {vertices[n][2 * t + 1].x, vertices[n][2 * t + 1].y, 0, 0});

      vAlphas.push_back(
          (1.0f -
           2.1 * fabs((float)(n - tailCounts[t] * 0.5f) / tailCounts[t])) *
          (1.f - fabs(alphas[t])));
      vAlphas.push_back(
          (1.0f -
           2.1 * fabs((float)(n - tailCounts[t] * 0.5f) / tailCounts[t])) *
          (1.0f - fabs(alphas[t])));
    }

    for (int n = 0; n < tailCount - 1; n++) {
      int vc = vBuf.size() - n * 2;
      iBuf.push_back(vc - 1);  // 1--2
      iBuf.push_back(vc - 2);  // |  |
      iBuf.push_back(vc - 3);  // 3--4
      iBuf.push_back(vc - 2);
      iBuf.push_back(vc - 4);
      iBuf.push_back(vc - 3);
    }
  }
  GL_CALL(glEnable(GL_BLEND));
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE));

  GL_CALL(glBindVertexArray(vao));

  GL_CALL(glGenBuffers(1, &vbo_alphas));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_alphas));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, vAlphas.size() * sizeof(GLfloat),
                       vAlphas.data(), GL_DYNAMIC_DRAW));
  GL_CALL(glEnableVertexAttribArray(1));
  GL_CALL(glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glGenBuffers(1, &vbo_vertices));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, vBuf.size() * sizeof(vec4), vBuf.data(),
                       GL_DYNAMIC_DRAW));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glUseProgram(tracer_shader));
  GL_CALL(glUniform2f(tracer_shader_aspect_ratio_uloc, ratio_x, ratio_y));
  GL_CALL(glUniform2f(tracer_shader_origin_uloc, x_offset, y_offset));

  GL_CALL(
      glDrawElements(GL_TRIANGLES, iBuf.size(), GL_UNSIGNED_INT, iBuf.data()));

  GL_CALL(glDeleteBuffers(1, &vbo_vertices));
  GL_CALL(glDeleteBuffers(1, &vbo_alphas));
}

float bilinearSample(float x, float y, float const* v, int nx, int ny) {
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

void advectTracers(vector<vec2>& tracers, float const* vx, float const* vy,
                   float dt, float h, int nx, int ny) {
  rotate(rbegin(vertices), rbegin(vertices) + 1, rend(vertices));
#pragma omp parallel for
  for (size_t i = 0; i < tracers.size(); i++) {
    const int steps = 4;
    float dth = dt / h / steps;
    auto& tracer = tracers[i];
    float oldX = tracer.x;
    float oldY = tracer.y;
    for (int step = 0; step < steps; step++) {
      float dx = bilinearSample(tracer.x, tracer.y, vx, nx, ny);
      float dy = bilinearSample(tracer.x, tracer.y, vy, nx, ny);
      float tX = tracer.x + 0.5f * dx * dth;
      float tY = tracer.y + 0.5f * dy * dth;
      tracer.x += bilinearSample(tX, tY, vx, nx, ny) * dth;
      tracer.y += bilinearSample(tX, tY, vy, nx, ny) * dth;
    }

    float dy = oldX - tracer.x;
    float dx = -(oldY - tracer.y);

    float len = sqrt(dx * dx + dy * dy);
    dx = dx / len * 1;
    dy = dy / len * 1;

    vertices[0][2 * i + 0] = {tracer.x + dx, tracer.y + dy};
    vertices[0][2 * i + 1] = {tracer.x - dx, tracer.y - dy};
  }
}

void pegToOne(float& xOut, float& yOut, float xIn, float yIn) {
  xOut = xIn / max(xIn, yIn);
  yOut = yIn / max(xIn, yIn);
}
void draw(float* vx, float* vy, float* flag, int nx, int ny, int screen_width,
          int screen_height, float scale, float dt, float h) {
  float screen_ratio_x = 0;
  float screen_ratio_y = 0;
  pegToOne(screen_ratio_x, screen_ratio_y,
           (float)screen_height / screen_width * nx / ny, 1.0f);
  float ratio_x = 2.0 * screen_ratio_x / nx;
  float ratio_y = 2.0 * screen_ratio_y / ny;

  for (int i = 0; i < 2; i++) {
    advectTracers(tracers, vx, vy, dt * 0.5, h, nx, ny);
  }

  drawTracers(ratio_x * scale, ratio_y * scale, -1.0f * screen_ratio_x * scale,
              -1.0f * screen_ratio_y * scale);

  for (size_t t = 0; t < tracerCount; t++) {
    float x = tracers[t].x;
    float y = tracers[t].y;

    if (x < 1.0 || x > nx - 1 || y < 1.0 || y > ny - 1 || alphas[t] < -1.0) {
      float tx = dis(gen) * nx;
      float ty = dis(gen) * ny;
      tracers[t] = {tx, ty};
      tailCounts[t] = 0;
      if (alphas[t] < -5)
        alphas[t] = dis(gen) * 2.0 - 1.0;
      else
        alphas[t] = 1.0;
      for (int n = 0; n < tailCount; n++) {
        vertices[n][2 * t + 0] = {tx, ty};
        vertices[n][2 * t + 1] = {tx, ty};
      }
    }

    tailCounts[t] = min(tailCounts[t] + 1, tailCount - 1);
    alphas[t] -= 0.01;
  }
}
}
