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
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_start_pointers);
    glBufferData(GL_SHADER_STORAGE_BUFFER, pointers.size() * sizeof(int),
                 pointers.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_end_pointers);
    glBufferData(GL_SHADER_STORAGE_BUFFER, pointers.size() * sizeof(int),
                 pointers.data(), GL_STATIC_DRAW);

    vector<int> players(ntracers, -1);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_players);
    glBufferData(GL_SHADER_STORAGE_BUFFER, players.size() * sizeof(int),
                 players.data(), GL_STATIC_DRAW);

    // allocate age SSBO with random values
    vector<float> ages(ntracers, 0.0);
    for (int i = 0; i < ntracers; i++) {
      ages[i] = 2 * 3.1; //(rand() % 10000) / 10000.0 * 2.0 * 3.141;
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_ages);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ages.size() * sizeof(float),
                 ages.data(), GL_STATIC_DRAW);

    // allocate index buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_indices);
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
    glBufferData(GL_SHADER_STORAGE_BUFFER, indices.size() * sizeof(int),
                 indices.data(), GL_STATIC_DRAW);
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

Shader advect_points_cs, update_vertices_cs, draw_tracers_sh;

GLTracers fluid_tracers, player_tracers;

void init() {

  advect_points_cs = Shader(loadComputeShader("advect_tracer_points.cs"));
  update_vertices_cs = Shader(loadComputeShader("update_tracer_vertices.cs"));
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
  registry.view<CoItem, CoPlayerAligned, CoHasTracer>().less(
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
        if (registry.has<CoPlayer>(ent))
          tracer.width = 1.0;
        else
          tracer.width = 0.2;
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
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, player_tracers.ssbo_points);
  glBufferData(GL_SHADER_STORAGE_BUFFER, points_buf.size() * sizeof(vec2),
               points_buf.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, player_tracers.ssbo_start_pointers);
  glBufferData(GL_SHADER_STORAGE_BUFFER,
               start_pointers_buf.size() * sizeof(vec2),
               start_pointers_buf.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, player_tracers.ssbo_end_pointers);
  glBufferData(GL_SHADER_STORAGE_BUFFER, end_pointers_buf.size() * sizeof(vec2),
               end_pointers_buf.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, player_tracers.ssbo_ages);
  glBufferData(GL_SHADER_STORAGE_BUFFER, ages_buf.size() * sizeof(vec2),
               ages_buf.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, player_tracers.ssbo_players);
  glBufferData(GL_SHADER_STORAGE_BUFFER, players_buf.size() * sizeof(int),
               players_buf.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, player_tracers.ssbo_vertices);
  glBufferData(GL_SHADER_STORAGE_BUFFER, vertices_buf.size() * sizeof(vec2),
               vertices_buf.data(), GL_STATIC_DRAW);
}

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
