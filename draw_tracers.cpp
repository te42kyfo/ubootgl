#include "draw_tracers.hpp"
#include "gl_error.hpp"
#include "load_shader.hpp"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/fast_square_root.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <iostream>
#include <random>

using namespace std;
using glm::vec2;
using glm::vec4;

namespace DrawTracers {

GLuint tracer_shader;
GLuint vao, vbo_vertices, vbo_alphas, vbo_players;
GLint tracer_shader_TM_uloc;

int frameNumber = 0;
const int tracerCount = 4000;
const int tailCount = 20;
vector<vector<vec2>> tracers(tailCount, vector<vec2>(tracerCount, {0, 0}));
vector<vector<vec2>> playerTracers(tailCount, vector<vec2>(1, {0, 0}));
vector<float> alphas(tracerCount, -10.0f);

vector<float> playerAlphas(1, 0.1f);
vector<int> tracerTailCounts(tracerCount);
vector<int> playerTailCounts(1);

default_random_engine gen;
uniform_real_distribution<float> dis(0.0, 1.0);

void init() {
  tracer_shader =
      loadShader("./scale_translate2D_alpha.vert", "./color.frag",
                 {{0, "in_Position"}, {1, "in_Alpha"}, {2, "in_Player"}});
  tracer_shader_TM_uloc = glGetUniformLocation(tracer_shader, "TM");

  GL_CALL(glGenVertexArrays(1, &vao));

  GL_CALL(glGenBuffers(1, &vbo_alphas));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_alphas));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                       tracerCount * tailCount * 2 * sizeof(float), NULL,
                       GL_STREAM_DRAW));

  GL_CALL(glGenBuffers(1, &vbo_vertices));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                       tracerCount * tailCount * 2 * sizeof(vec4), NULL,
                       GL_STREAM_DRAW));

  GL_CALL(glGenBuffers(1, &vbo_players));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_players));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                       tracerCount * tailCount * 2 * sizeof(int), NULL,
                       GL_STREAM_DRAW));
};

void drawTracers(const vector<vector<vec2>> &tracers,
                 const vector<float> &alphas, const vector<int> tailCounts,
                 glm::mat4 TM, bool players) {
  vector<vec4> vertices;
  vector<int> indices;
  vector<float> vAlphas;
  vector<int> players_buf;
  vector<float> sides;
  for (size_t t = 0; t < tracers[0].size(); t++) {

    glm::vec2 screenPos1 = TM * glm::vec4(tracers[0][t], 0.0f, 1.0f);
    glm::vec2 screenPos2 = TM * glm::vec4(tracers[tailCount - 1][t], 0.0, 1.0);

    if (!(screenPos1.x > -1.0 && screenPos1.x < 1.0 && screenPos1.y > -1.0 &&
          screenPos1.y < 1.0) &&
        !(screenPos2.x > -1.0 && screenPos2.x < 1.0 && screenPos2.y > -1.0 &&
          screenPos2.y < 1.0)) {
      continue;
    }
    for (int n = 0; n < tailCount; n++) {
      vec2 normal;
      if (n < tailCount - 1) {
        normal = tracers[n][t] - tracers[n + 1][t];
        //        cout << tracers[n][t].x << " " << tracers[n + 1][t].y << "\n";
      }
      if (n == tailCount - 1)
        normal = tracers[n - 1][t] - tracers[n][t];

      //    cout << normal.x << " " << normal.y << "\n";
      swap(normal.x, normal.y);
      normal.x *= -1;

      normal = glm::fastNormalize(normal);

      float tracerWidth = 0.2f;
      if (players)
        tracerWidth = 0.5f;

      vertices.push_back(
          vec4(tracers[n][t] + normal * tracerWidth, 0.0f, 1.0f));
      vertices.push_back(
          vec4(tracers[n][t] - normal * tracerWidth, 0.0f, 1.0f));

      float alpha = (tailCounts[t] - n) / (max(1.0f, tailCounts[t] - 1.0f)) *
                    (1.f - fabs(alphas[t]));

      vAlphas.push_back(alpha);
      vAlphas.push_back(alpha);

      if (players) {
        players_buf.push_back(t);
        players_buf.push_back(t);
      } else {
        players_buf.push_back(-1);
        players_buf.push_back(-1);
      }
    }

    for (int n = 0; n < tailCount - 1; n++) {
      int vc = vertices.size() - n * 2;
      glm::vec2 screenPos = TM * vertices[vc - 1];
      if (screenPos.x > -1.1 && screenPos.x < 1.1 && screenPos.y > -1.1 &&
          screenPos.y < 1.1) {
        indices.push_back(vc - 1); // 1--2
        indices.push_back(vc - 2); // |  |
        indices.push_back(vc - 3); // 3--4
        indices.push_back(vc - 2);
        indices.push_back(vc - 4);
        indices.push_back(vc - 3);
      }
    }
  }

  GL_CALL(glEnable(GL_BLEND));

  if (players)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  else
    GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE));

  GL_CALL(glBindVertexArray(vao));

  // player VBO
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_players));
  GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, players_buf.size() * sizeof(int),
                          players_buf.data()));
  GL_CALL(glEnableVertexAttribArray(2));
  GL_CALL(glVertexAttribIPointer(2, 1, GL_INT, 0, 0));

  // Alpha VBO
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_alphas));
  GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, vAlphas.size() * sizeof(GLfloat),
                          vAlphas.data()));
  GL_CALL(glEnableVertexAttribArray(1));
  GL_CALL(glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0));

  // Vertex VBO
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices));
  GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(vec4),
                          vertices.data()));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));

  // unifrom parameters
  GL_CALL(glUseProgram(tracer_shader));
  GL_CALL(glUniformMatrix4fv(tracer_shader_TM_uloc, 1, GL_FALSE,
                             glm::value_ptr(TM)));

  // draw
  GL_CALL(glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT,
                         indices.data()));

  GL_CALL(glDisableVertexAttribArray(0));
  GL_CALL(glDisableVertexAttribArray(1));
}

float bilinearSample(float x, float y, float const *v, int nx, int ny) {
  int ix = floor(x - 0.5f);
  int iy = floor(y - 0.5f);
  if (ix < 0 || ix >= nx - 1 || iy < 0 || iy >= ny - 1)
    return 0.0f;

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

void advectTracers(vector<vec2> &tracers, float const *vx, float const *vy,
                   float dt, float h, int nx, int ny) {
#pragma omp parallel for
  for (size_t i = 0; i < tracers.size(); i++) {
    auto &tracer = tracers[i];
    const int steps = 4;
    float dth = dt / h / steps;
    for (int step = 0; step < steps; step++) {
      float dx = bilinearSample(tracer.x, tracer.y, vx, nx, ny);
      float dy = bilinearSample(tracer.x, tracer.y, vy, nx, ny);
      float tX = tracer.x + 0.5 * dx * dth;
      float tY = tracer.y + 0.5 * dy * dth;
      tracer.x += bilinearSample(tX, tY, vx, nx, ny) * dth;
      tracer.y += bilinearSample(tX, tY, vy, nx, ny) * dth;
    }
  }
}

void pegToOne(float &xOut, float &yOut, float xIn, float yIn) {
  xOut = xIn / max(xIn, yIn);
  yOut = yIn / max(xIn, yIn);
}

void playerTracersAdd(int pid, vec2 pos) {
  if (pid >= playerTracers[0].size()) {
    playerTracers =
        vector<vector<vec2>>(tailCount, vector<vec2>(pid + 1, {0, 0}));
    playerAlphas = vector<float>(pid + 1, -0.1f);
    playerTailCounts = vector<int>(pid + 1);
  }
  // cout << pid << " " << pos.x << " " << pos.y << "\n";
  playerTailCounts[pid] = min(playerTailCounts[pid] + 1, tailCount - 1);
  playerTracers[0][pid] = pos;
}

void updateTracers(float *vx, float *vy, float *flag, int nx, int ny, float dt,
                   float pwidth) {

  float h = pwidth / (nx - 1);

  frameNumber++;
  if (frameNumber == 2) {
    rotate(rbegin(tracers), rbegin(tracers) + 1, rend(tracers));
    rotate(rbegin(playerTracers), rbegin(playerTracers) + 1,
           rend(playerTracers));
    tracers[0] = tracers[1];
    playerTracers[0] = playerTracers[1];
    frameNumber = 0;
  }

  advectTracers(tracers[0], vx, vy, dt, h, nx, ny);

  for (size_t t = 0; t < tracerCount; t++) {
    float x = tracers[tailCount - 1][t].x;
    float y = tracers[tailCount - 1][t].y;

    if (x < 1.0 || x > nx - 1 || y < 1.0 || y > ny - 1 || alphas[t] < -1.0 ||
        bilinearSample(x, y, flag, nx, ny) < 0.8) {
      float tx = dis(gen) * nx;
      float ty = dis(gen) * ny;
      while (bilinearSample(tx, ty, flag, nx, ny) == 0) {
        tx = dis(gen) * nx;
        ty = dis(gen) * ny;
      }

      tracers[0][t] = {tx, ty};
      for (int n = 1; n < tailCount; n++) {
        tracers[n][t] = {tx, ty};
      }
      tracerTailCounts[t] = 0;
      if (alphas[t] < -5)
        alphas[t] = dis(gen) * 2.0 - 1.0;
      else
        alphas[t] = 1.0;
    }
    tracerTailCounts[t] = min(tracerTailCounts[t] + 1, tailCount - 1);
    alphas[t] -= 0.001;
  }
}

void draw(int nx, int ny, glm::mat4 PVM, float pwidth) {

  // Model
  glm::mat4 TM = glm::scale(PVM, glm::vec3(pwidth / nx, pwidth / nx, pwidth));

  drawTracers(tracers, alphas, tracerTailCounts, TM, false);
  drawTracers(playerTracers, playerAlphas, playerTailCounts, TM, true);
}
} // namespace DrawTracers
