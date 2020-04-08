#include "draw_tracers_cs.hpp"
#include "GL/glew.h"
#include "gl_error.hpp"
#include "load_shader.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

using namespace std;
using vec2 = glm::vec2;
namespace DrawTracersCS {
GLuint tracer_shader, compute_shader;
GLuint vao;
GLuint tracer_shader_TM;
GLuint ssbo_points, ssbo_vertices;

vector<unsigned int> indices;

const int npoints = 40;
const int ntracers = 2;

void init() {

  tracer_shader =
      loadShader("./tracercs.vert", "./color.frag", {{0, "in_Position"}});
  tracer_shader_TM = glGetUniformLocation(tracer_shader, "TM");

  compute_shader = loadComputeShader("tracers.cs");

  GL_CALL(glGenVertexArrays(1, &vao));
  glGenBuffers(1, &ssbo_points);
  glGenBuffers(1, &ssbo_vertices);

  vector<vec2> pointdata;
  for (int t = 0; t < ntracers; t++) {
    for (int p = 0; p < npoints; p++) {
      pointdata.push_back(
          vec2(100 + p * 10.2, 100.0 + sin(p * 0.2) * 10.2 + t * 1.2));
    }
  }
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_points);
  glBufferData(GL_SHADER_STORAGE_BUFFER, pointdata.size() * sizeof(vec2),
               pointdata.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vertices);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               sizeof(glm::vec2) * npoints * ntracers * 2, NULL,
               GL_STATIC_DRAW);

  // generate indices for triangle strip once
  for (int t = 0; t < ntracers; t++) {
    int p = 0;
    indices.push_back(p * 2);
    indices.push_back(p * 2 + 1);
    indices.push_back(p * 2 + 2);
    for (p = 1; p < npoints - 1; p++) {
      indices.push_back(p * 2 - 2);
      indices.push_back(p * 2);
      indices.push_back(p * 2 + 1);

      indices.push_back(p * 2);
      indices.push_back(p * 2 + 1);
      indices.push_back(p * 2 + 2);
    }

    indices.push_back(p * 2 - 2);
    indices.push_back(p * 2);
    indices.push_back(p * 2 + 1);
  }
};

void updateTracers(float *vx, float *vy, float *flag, int nx, int ny, float dt,
                   float pwidth){};

void draw(int nx, int ny, glm::mat4 PVM, float pwidth) {

  // Model
  glm::mat4 TM = glm::scale(PVM, glm::vec3(pwidth / nx, pwidth / nx, pwidth));

  GL_CALL(glBindVertexArray(vao));

  // compute vertices
  GL_CALL(glUseProgram(compute_shader));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_points));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_vertices));
  GL_CALL(glDispatchCompute((npoints - 1) / 256 + 1, 1, 1));
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // Use vertex SSBO as vertex attrib ponter
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, ssbo_vertices));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glUseProgram(tracer_shader));
  GL_CALL(
      glUniformMatrix4fv(tracer_shader_TM, 1, GL_FALSE, glm::value_ptr(TM)));

  GL_CALL(glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT,
                         indices.data()));

  GL_CALL(glDisableVertexAttribArray(0));
};

} // namespace DrawTracersCS
