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
vector<vector<vec4>> tracers(tailCount,
                             vector<vec4>(tracerCount, {0, 0, 0, 0}));
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

void drawTracers(const vector<vector<vec4>>& tracers,
                 const vector<float>& alphas, float ratio_x, float ratio_y,
                 float x_offset, float y_offset) {
  vector<vec4> vertices;
  vector<int> indices;
  vector<float> vAlphas;
  for (int t = 0; t < tracers[0].size(); t++) {
    for (int n = 0; n < tailCount; n++) {
      float dx, dy;
      if (n < tailCount - 1) {
        dy = -(tracers[n][t].x - tracers[n + 1][t].x);
        dx = tracers[n][t].y - tracers[n + 1][t].y;
      }
      if (n == tailCount - 1) {
        dy = -(tracers[n - 1][t].x - tracers[n][t].x);
        dx = tracers[n - 1][t].y - tracers[n][t].y;
      }
      float len = sqrt(dx * dx + dy * dy);
      dx = dx / len * ratio_x * 2;
      dy = dy / len * ratio_y * 2;

      vertices.push_back(
          {tracers[n][t].x + dx, tracers[n][t].y + dy, 0.0f, 1.0f});
      vertices.push_back(
          {tracers[n][t].x - dx, tracers[n][t].y - dy, 0.0f, 1.0f});

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
      int vc = vertices.size() - n * 2;
      indices.push_back(vc - 1);  // 1--2
      indices.push_back(vc - 2);  // |  |
      indices.push_back(vc - 3);  // 3--4
      indices.push_back(vc - 2);
      indices.push_back(vc - 4);
      indices.push_back(vc - 3);
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
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vec4),
                       vertices.data(), GL_DYNAMIC_DRAW));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glUseProgram(tracer_shader));
  GL_CALL(glUniform2f(tracer_shader_aspect_ratio_uloc, ratio_x, ratio_y));
  GL_CALL(glUniform2f(tracer_shader_origin_uloc, x_offset, y_offset));

  GL_CALL(glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT,
                         indices.data()));

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

void advectTracers(vector<vec4>& tracersSrc, vector<vec4>& tracersDst,
                   float const* vx, float const* vy, float dt, float h, int nx,
                   int ny) {
#pragma omp parallel for
  for (size_t i = 0; i < tracersSrc.size(); i++) {
    auto& tracer = tracersSrc[i];
    float dx = bilinearSample(tracer.x, tracer.y, vx, nx, ny);
    float dy = bilinearSample(tracer.x, tracer.y, vy, nx, ny);

    float tX = tracer.x + 0.5 * dx * dt / h;
    float tY = tracer.y + 0.5 * dy * dt / h;

    tracersDst[i].x = tracer.x + bilinearSample(tX, tY, vx, nx, ny) * dt / h;
    tracersDst[i].y = tracer.y + bilinearSample(tX, tY, vy, nx, ny) * dt / h;
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
    rotate(rbegin(tracers), rbegin(tracers) + 1, rend(tracers));
    advectTracers(tracers[1], tracers[0], vx, vy, dt * 0.5, h, nx, ny);
  }
  drawTracers(tracers, alphas, ratio_x * scale, ratio_y * scale,
              -1.0f * screen_ratio_x * scale, -1.0f * screen_ratio_y * scale);

  for (size_t t = 0; t < tracerCount; t++) {
    float x = tracers[tailCount - 1][t].x;
    float y = tracers[tailCount - 1][t].y;

    if (x < 1.0 || x > nx - 1 || y < 1.0 || y > ny - 1 || alphas[t] < -1.0) {
      float tx = dis(gen) * nx;
      float ty = dis(gen) * ny;
      tracers[0][t] = {tx, ty};
      for (int n = 1; n < tailCount; n++) {
        tracers[n][t] = {tx, ty};
      }
      tailCounts[t] = 0;
      if (alphas[t] < -5)
        alphas[t] = dis(gen) * 2.0 - 1.0;
      else
        alphas[t] = 1.0;
    }
    tailCounts[t] = min(tailCounts[t] + 1, tailCount - 1);
    alphas[t] -= 0.01;
  }
}
}
