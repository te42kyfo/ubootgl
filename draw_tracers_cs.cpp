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
GLuint uloc_tracer_shader_TM, uloc_compute_shader_npoints,
    uloc_compute_shader_ntracers;
GLuint ssbo_points, ssbo_vertices, ssbo_pointers, ssbo_ages, ssbo_indices;

vector<unsigned int> indices;

const int npoints = 500;
const int ntracers = 10;

void init() {

  tracer_shader =
      loadShader("./tracercs.vert", "./color.frag", {{0, "in_Position"}});
  uloc_tracer_shader_TM = glGetUniformLocation(tracer_shader, "TM");

  compute_shader = loadComputeShader("tracers.cs");
  uloc_compute_shader_npoints = glGetUniformLocation(compute_shader, "npoints");
  uloc_compute_shader_ntracers =
      glGetUniformLocation(compute_shader, "ntracers");

  GL_CALL(glGenVertexArrays(1, &vao));
  glGenBuffers(1, &ssbo_points);
  glGenBuffers(1, &ssbo_vertices);
  glGenBuffers(1, &ssbo_pointers);
  glGenBuffers(1, &ssbo_ages);
  glGenBuffers(1, &ssbo_indices);

  // initialize points SSBO
  vector<vec2> pointdata;
  for (int t = 0; t < ntracers; t++) {
    for (int p = 0; p < npoints; p++) {
      pointdata.push_back(vec2(10 * t, 200));
    }
  }
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_points);
  glBufferData(GL_SHADER_STORAGE_BUFFER, pointdata.size() * sizeof(vec2),
               pointdata.data(), GL_STATIC_DRAW);

  // allocate vertices SSBO with zeros
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vertices);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               sizeof(glm::vec2) * npoints * ntracers * 2, NULL,
               GL_STATIC_DRAW);

  // allocate pointers SSBO with zeros
  vector<int> pointers(ntracers, 0);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_pointers);
  glBufferData(GL_SHADER_STORAGE_BUFFER, pointers.size() * sizeof(int),
               pointers.data(), GL_STATIC_DRAW);

  // allocate pointers SSBO with zeros
  vector<float> ages(ntracers, 0.0);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_ages);
  glBufferData(GL_SHADER_STORAGE_BUFFER, ages.size() * sizeof(float),
               ages.data(), GL_STATIC_DRAW);

  // allocate index buffer
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_indices);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               ntracers * (npoints - 1) * 6 * sizeof(int), NULL,
               GL_STATIC_DRAW);
};

void updateTracers(float *vx, float *vy, float *flag, int nx, int ny, float dt,
                   float pwidth) {

  // compute vertices
  GL_CALL(glUseProgram(compute_shader));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_points));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_vertices));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_pointers));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_ages));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_indices));
  GL_CALL(glUniform1i(uloc_compute_shader_npoints, npoints));
  GL_CALL(glUniform1i(uloc_compute_shader_ntracers, ntracers));

  GL_CALL(glDispatchCompute((ntracers - 1) / 256 + 1, 1, 1));
};

void draw(int nx, int ny, glm::mat4 PVM, float pwidth) {

  // Model
  glm::mat4 TM = glm::scale(PVM, glm::vec3(pwidth / nx, pwidth / nx, pwidth));

  GL_CALL(glBindVertexArray(vao));

  // Use vertex SSBO as vertex attrib ponter
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, ssbo_vertices));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glUseProgram(tracer_shader));
  GL_CALL(glUniformMatrix4fv(uloc_tracer_shader_TM, 1, GL_FALSE,
                             glm::value_ptr(TM)));

  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ssbo_indices));

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  GL_CALL(glDrawElements(GL_TRIANGLES, ntracers * (npoints - 1) * 6,
                         GL_UNSIGNED_INT, NULL));

  GL_CALL(glDisableVertexAttribArray(0));
};

} // namespace DrawTracersCS
