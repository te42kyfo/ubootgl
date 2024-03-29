#include "draw_tracers_cs.hpp"
#include "GL/glew.h"
#include "components.hpp"
#include "gl_error.hpp"
#include "load_shader.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <vector>

using namespace std;
using vec2 = glm::vec2;
namespace DrawTracersCS {

template <typename T> size_t vectorBytes(std::vector<T> &vec) {
  return sizeof(T) * vec.size();
}

template <typename T> void glVectorToSSBO(GLuint target, std::vector<T> &vec) {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, target);
  glBufferData(GL_SHADER_STORAGE_BUFFER, vectorBytes(vec), vec.data(),
               GL_STATIC_DRAW);
  GL_CALL(glGetError());
}

class GLTracers {
public:
  void init(int _npoints, int _ntracers) {

    npoints = _npoints;
    ntracers = _ntracers;
    GL_CALL(glGenVertexArrays(1, &vao));
    glGenBuffers(1, &ssbo_points);
    glGenBuffers(1, &ssbo_vertices);
    glGenBuffers(1, &ssbo_start_pointers);
    glGenBuffers(1, &ssbo_end_pointers);
    glGenBuffers(1, &ssbo_ages);
    glGenBuffers(1, &ssbo_indices);
    glGenBuffers(1, &ssbo_players);

    // initialize points SSBO
    vector<vec2> pointdata(ntracers * npoints, vec2(0, 0));
    glVectorToSSBO(ssbo_points, pointdata);

    // allocate vertices SSBO with zeros
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_vertices);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 sizeof(glm::vec2) * npoints * ntracers * 2, NULL,
                 GL_STATIC_DRAW);

    // allocate pointers SSBO with zeros
    vector<int> pointers(ntracers, 0);
    glVectorToSSBO(ssbo_start_pointers, pointers);
    glVectorToSSBO(ssbo_end_pointers, pointers);

    vector<int> players(ntracers, -1);
    glVectorToSSBO(ssbo_players, players);

    // allocate age SSBO with random values
    vector<float> ages(ntracers, 0.0);
    for (int i = 0; i < ntracers; i++) {
      ages[i] = 2 * 3.1; //(rand() % 10000) / 10000.0 * 2.0 * 3.141;
    }
    glVectorToSSBO(ssbo_ages, ages);

    // allocate index buffer
    vector<int> indices(ntracers * (npoints - 1) * 6);
    for (int nt = 0; nt < ntracers; nt++) {
      for (int np = 0; np < npoints - 1; np++) {
        int base = (nt * npoints + np) * 2;
        indices[(nt * (npoints - 1) + np) * 6 + 0] = base + 0;
        indices[(nt * (npoints - 1) + np) * 6 + 1] = base + 1;
        indices[(nt * (npoints - 1) + np) * 6 + 2] = base + 2;
        indices[(nt * (npoints - 1) + np) * 6 + 3] = base + 1;
        indices[(nt * (npoints - 1) + np) * 6 + 4] = base + 2;
        indices[(nt * (npoints - 1) + np) * 6 + 5] = base + 3;
      }
    }

    glVectorToSSBO(ssbo_indices, indices);
  }

  void bindSSBOs() {
    GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_points));
    GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_vertices));
    GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_start_pointers));
    GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_end_pointers));
    GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_ages));
    GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_indices));
  }

  GLuint ssbo_points, ssbo_vertices, ssbo_start_pointers, ssbo_ages,
      ssbo_indices, ssbo_end_pointers, ssbo_players;

  GLuint vao;
  int npoints = 30;
  int ntracers = 1000;
};
const int nPlayerPoints = 100;

struct Tracer {
  Tracer() : points(nPlayerPoints), end_pointer(0), start_pointer(0) {}
  std::vector<vec2> points;
  int end_pointer;
  int start_pointer;
  float age = 3.141;
  float width = 1.0f;
  int player = -1;
};

std::map<entt ::entity, Tracer> playerPoints;

Shader advect_points_cs, update_vertices_cs, draw_tracers_sh, shift_tracers_cs;

GLTracers fluid_tracers, player_tracers;

void init() {

  advect_points_cs = Shader(loadComputeShader("advect_tracer_points.cs"));
  update_vertices_cs = Shader(loadComputeShader("update_tracer_vertices.cs"));
  shift_tracers_cs = Shader(loadComputeShader("shift_tracers.cs"));

  draw_tracers_sh = Shader(
      loadShader("./tracercs.vert", "./color.frag", {{0, "in_Position"}}));

  fluid_tracers.init(30, 1000);
  player_tracers.init(nPlayerPoints, 100);
};

void updateTracers(GLuint tex_vxy, GLuint flag_tex, int nx, int ny, float dt,
                   float pwidth) {

  // advect the points with the fluid
  fluid_tracers.bindSSBOs();
  GL_CALL(glUseProgram(advect_points_cs.id));
  GL_CALL(glUniform1i(advect_points_cs.uloc("npoints"), fluid_tracers.npoints));
  GL_CALL(
      glUniform1i(advect_points_cs.uloc("ntracers"), fluid_tracers.ntracers));
  GL_CALL(glUniform1f(advect_points_cs.uloc("dt"), dt));
  GL_CALL(glUniform1ui(advect_points_cs.uloc("rand_seed"), rand()));
  static float angle = 0.0;
  angle += 0.1;
  GL_CALL(glUniform1f(advect_points_cs.uloc("angle"), angle));
  GL_CALL(glUniform2f(advect_points_cs.uloc("pdim"), pwidth, pwidth * ny / nx));

  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, tex_vxy));
  GL_CALL(glUniform1i(advect_points_cs.uloc("tex_vxy"), 0));

  GL_CALL(glActiveTexture(GL_TEXTURE1));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, flag_tex));
  GL_CALL(glUniform1i(advect_points_cs.uloc("tex_flag"), 1));

  GL_CALL(glDispatchCompute((fluid_tracers.ntracers - 1) / 256 + 1, 1, 1));

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  // compute new vertices and indices
  GL_CALL(glUseProgram(update_vertices_cs.id));
  fluid_tracers.bindSSBOs();
  GL_CALL(
      glUniform1i(update_vertices_cs.uloc("npoints"), fluid_tracers.npoints));
  GL_CALL(
      glUniform1i(update_vertices_cs.uloc("ntracers"), fluid_tracers.ntracers));
  GL_CALL(
      glUniform2f(update_vertices_cs.uloc("pdim"), pwidth, pwidth * ny / nx));

  GL_CALL(glDispatchCompute((fluid_tracers.ntracers - 1) / 256 + 1, 1, 1));
};

void updatePlayerTracers(entt::registry &registry) {

  // update new item positions into tracer point arrays
  registry.view<CoItem, CoPlayerAligned, CoHasTracer>().each(
      [&](auto ent, auto &item, auto &player) {
        if (playerPoints.count(ent) == 0) {
          playerPoints[ent] = Tracer();
          playerPoints[ent].points[0] = item.pos;
          playerPoints[ent].player =
              registry.get<CoPlayer>(player.player).keySet;
        }
        Tracer &tracer = playerPoints[ent];
        tracer.end_pointer = (tracer.end_pointer + 1) % nPlayerPoints;
        if (tracer.start_pointer == tracer.end_pointer)
          tracer.start_pointer = (tracer.start_pointer + 1) % nPlayerPoints;

        tracer.points[tracer.end_pointer] =
            item.pos -
            item.size[0] * vec2(cos(item.rotation), sin(item.rotation)) * 0.5f;
        tracer.age = 3.141;
        if (registry.all_of<CoPlayer>(ent))
          tracer.width = 3.0;
        else
          tracer.width = 0.8;
      });

  for (auto it = begin(playerPoints); it != end(playerPoints);) {
    it->second.age -= 0.02;
    if (it->second.age < 0.0) {
      it = playerPoints.erase(it);
    } else {
      it++;
    }
  }

  // build buffers for GPU upload
  std::vector<vec2> points_buf;
  std::vector<int> start_pointers_buf;
  std::vector<int> end_pointers_buf;
  std::vector<float> ages_buf;
  std::vector<vec2> vertices_buf;
  std::vector<int> players_buf;
  for (auto tracer : playerPoints) {
    int start_pointer = tracer.second.start_pointer;
    int end_pointer = tracer.second.end_pointer;
    int npoints = player_tracers.npoints;
    std::vector<vec2> this_vertices(npoints * 2, vec2(0, 0));
    for (auto point : tracer.second.points) {
      points_buf.push_back(point);
    }

    int tracer_length = (end_pointer - start_pointer + npoints) % npoints;
    int base = points_buf.size() - npoints;
    if (tracer_length > 0) {
      vec2 perp = normalize(points_buf[base + start_pointer] -
                            points_buf[base + (start_pointer + 1) % npoints]);
      this_vertices[start_pointer * 2 + 0] = points_buf[base + start_pointer];
      this_vertices[start_pointer * 2 + 1] = points_buf[base + start_pointer];

      for (int i = 1; i < tracer_length; i++) {
        int curr = (start_pointer + i) % npoints;
        int next = (curr + 1) % npoints;
        int prev = (curr - 1 + npoints) % npoints;

        vec2 v1 = points_buf[base + curr] - points_buf[base + prev];
        vec2 v2 = points_buf[base + next] - points_buf[base + curr];

        float width =
            tracer.second.width * (0.00001f + (length(v1) + length(v2)) * 0.4f);
        if (width > 0.1)
          width = 0.0;
        perp = normalize(vec2(v1.y, -v1.x) + vec2(v2.y, -v2.x) * 0.5f);

        this_vertices[curr * 2 + 0] = points_buf[base + curr] + perp * width;
        this_vertices[curr * 2 + 1] = points_buf[base + curr] - perp * width;
      }
      perp =
          normalize(points_buf[base + end_pointer] -
                    points_buf[base + (end_pointer + npoints - 1) % npoints]);
      this_vertices[end_pointer * 2 + 0] = points_buf[base + end_pointer];
      this_vertices[end_pointer * 2 + 1] = points_buf[base + end_pointer];
    }
    vertices_buf.insert(end(vertices_buf), begin(this_vertices),
                        end(this_vertices));

    start_pointers_buf.push_back(tracer.second.start_pointer);
    end_pointers_buf.push_back(tracer.second.end_pointer);
    ages_buf.push_back(tracer.second.age);
    players_buf.push_back(tracer.second.player);
  }

  player_tracers.ntracers = start_pointers_buf.size();

  // upload buffers
  glVectorToSSBO(player_tracers.ssbo_points, points_buf);
  glVectorToSSBO(player_tracers.ssbo_start_pointers, start_pointers_buf);
  glVectorToSSBO(player_tracers.ssbo_end_pointers, end_pointers_buf);
  glVectorToSSBO(player_tracers.ssbo_ages, ages_buf);
  glVectorToSSBO(player_tracers.ssbo_players, players_buf);
  glVectorToSSBO(player_tracers.ssbo_vertices, vertices_buf);
}

void shiftTracers(GLTracers &tracers, float shift) {

  tracers.bindSSBOs();
  GL_CALL(glUseProgram(shift_tracers_cs.id));
  GL_CALL(glUniform1i(shift_tracers_cs.uloc("npoints"), tracers.npoints));
  GL_CALL(glUniform1i(shift_tracers_cs.uloc("ntracers"), tracers.ntracers));
  GL_CALL(glUniform1f(shift_tracers_cs.uloc("shift"), shift));

  GL_CALL(glDispatchCompute((tracers.ntracers * tracers.npoints - 1) / 256 + 1,
                            1, 1));
  GL_CALL(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));
}

  void shiftPlayerTracers(float shift) {

    for(auto& tracer : playerPoints) {
      for ( auto& point : tracer.second.points ) {
        point += vec2(shift, 0.0f);
      }
    }

  }
  
void shiftFluidTracers(float shift) { shiftTracers(fluid_tracers, shift); }

void draw(GLTracers &tracers, glm::mat4 PVM) {
  GL_CALL(glBindVertexArray(tracers.vao));

  GL_CALL(glUseProgram(draw_tracers_sh.id));
  GL_CALL(glUniformMatrix4fv(draw_tracers_sh.uloc("TM"), 1, GL_FALSE,
                             glm::value_ptr(PVM)));
  GL_CALL(glUniform1i(draw_tracers_sh.uloc("npoints"), tracers.npoints));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
                           tracers.ssbo_start_pointers));
  GL_CALL(
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, tracers.ssbo_end_pointers));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, tracers.ssbo_ages));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, tracers.ssbo_vertices));
  GL_CALL(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, tracers.ssbo_players));
  GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tracers.ssbo_indices));

  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  GL_CALL(glDrawElements(GL_TRIANGLES,
                         tracers.ntracers * (tracers.npoints - 1) * 6,
                         GL_UNSIGNED_INT, NULL));

  GL_CALL(glDisableVertexAttribArray(0));
};

void drawPlayerTracers(glm::mat4 PVM) { draw(player_tracers, PVM); }
void draw(glm::mat4 PVM) { draw(fluid_tracers, PVM); }

} // namespace DrawTracersCS
