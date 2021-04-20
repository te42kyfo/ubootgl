#include "draw_floating_items.hpp"
#include "gl_error.hpp"
#include "load_shader.hpp"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <map>
#include <vector>

namespace DrawFloatingItems {

using namespace std;

GLuint item_shader;
GLuint vao;
GLint item_shader_frameGridSize_uloc, item_shader_PVM_uloc;
GLuint pos_ssbo;
GLuint size_ssbo;
GLuint rot_ssbo;
GLuint frame_ssbo;
GLuint player_ssbo;
vector<GLuint> idx_buf;

void init() {
  item_shader = loadShader("./sprite.vert", "./sprite.frag", {});
  item_shader_frameGridSize_uloc =
      glGetUniformLocation(item_shader, "frameGridSize");

  item_shader_PVM_uloc = glGetUniformLocation(item_shader, "PVM");

  // Quad Geometry
  GL_CALL(glGenVertexArrays(1, &vao));
  GL_CALL(glBindVertexArray(vao));

  glGenBuffers(1, &pos_ssbo);
  glGenBuffers(1, &size_ssbo);
  glGenBuffers(1, &rot_ssbo);
  glGenBuffers(1, &frame_ssbo);
  glGenBuffers(1, &player_ssbo);
}

void draw(entt::registry &registry, entt::id_type component, Texture texture,
          glm::mat4 PVM, float magnification, bool blendSum, bool highlight) {

  entt::id_type components[] = {component, entt::type_hash<CoItem>::value()};
  auto spriteView = registry.runtime_view(cbegin(components), cend(components));
  if (begin(spriteView) == end(spriteView))
    return;

  vector<glm::vec2> pos_buf;
  vector<glm::vec2> size_buf;
  vector<float> frame_buf;
  vector<float> rot_buf;
  vector<int> player_buf;

  auto inversePVM = glm::inverse(PVM);
  auto screenP1 = inversePVM * glm::vec4(-1.0, -1.0, 0.0, 1.0);
  auto screenP2 = inversePVM * glm::vec4(1.0, 1.0, 0.0, 1.0);

  for (auto entity : spriteView) {
    auto item = registry.get<CoItem>(entity);

    if (item.pos.x > screenP1.x && item.pos.x < screenP2.x &&
        item.pos.y > screenP1.y && item.pos.y < screenP2.y) {
      pos_buf.push_back(item.pos);

      if (highlight)
        item.size += min(item.size[0], item.size[1]) * 0.3f;

      size_buf.push_back(item.size * magnification);
      rot_buf.push_back(item.rotation);

      auto* f = registry.try_get<CoAnimated>(entity);
      if (f) {
        frame_buf.push_back(f->frame);
      }

      auto* p = registry.try_get<CoPlayerAligned>(entity);
      if (p && highlight) {
        player_buf.push_back(registry.get<CoPlayer>(p->player).keySet);
      } else {
        player_buf.push_back(-1);
      }

    }
  }

  if (idx_buf.size() < pos_buf.size() * 6) {
    for (int i = (int)idx_buf.size() / 6; i < (int)pos_buf.size() * 6; i++) {
      idx_buf.push_back(i * 4 + 0);
      idx_buf.push_back(i * 4 + 1);
      idx_buf.push_back(i * 4 + 2);
      idx_buf.push_back(i * 4 + 2);
      idx_buf.push_back(i * 4 + 3);
      idx_buf.push_back(i * 4 + 0);
    }
  }

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, pos_ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, pos_buf.size() * sizeof(glm::vec2),
               pos_buf.data(), GL_STREAM_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pos_ssbo);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, size_ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, size_buf.size() * sizeof(glm::vec2),
               size_buf.data(), GL_STREAM_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, size_ssbo);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, rot_ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, rot_buf.size() * sizeof(float),
               rot_buf.data(), GL_STREAM_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, rot_ssbo);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, frame_ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, frame_buf.size() * sizeof(float),
               frame_buf.data(), GL_STREAM_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, frame_ssbo);

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, player_ssbo);
  glBufferData(GL_SHADER_STORAGE_BUFFER, player_buf.size() * sizeof(int),
               player_buf.data(), GL_STREAM_DRAW);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, player_ssbo);

  GL_CALL(glUseProgram(item_shader));
  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, texture.tex_id));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR_MIPMAP_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

  if (blendSum) {
    GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE));
  } else {
    GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  }
  if (highlight) {
    GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, 1.5));
  } else {
    GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.2));
  }

  GL_CALL(glBindVertexArray(vao));

  GL_CALL(glUniform2i(item_shader_frameGridSize_uloc, texture.nx, texture.ny));
  GL_CALL(glUniformMatrix4fv(item_shader_PVM_uloc, 1, GL_FALSE,
                             glm::value_ptr(PVM)));

  GL_CALL(glDrawElements(GL_TRIANGLES, pos_buf.size() * 6, GL_UNSIGNED_INT,
                         idx_buf.data()));

} // namespace DrawFloatingItems

} // namespace DrawFloatingItems
