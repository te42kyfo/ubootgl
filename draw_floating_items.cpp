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
GLuint vao, position_vbo, frame_vbo, uv_vbo, color_vbo;
GLint item_shader_frameGridSize_uloc;

void init() {
  item_shader = loadShader("./sprite.vert", "./sprite.frag",
                           {{0, "in_Position"},
                            {1, "in_Frame"},
                            {2, "in_UV"},
                            {3, "in_PlayerColor"}});
  item_shader_frameGridSize_uloc =
      glGetUniformLocation(item_shader, "frameGridSize");

  // Quad Geometry
  GL_CALL(glGenVertexArrays(1, &vao));
  GL_CALL(glGenBuffers(1, &position_vbo));
  GL_CALL(glGenBuffers(1, &frame_vbo));
  GL_CALL(glGenBuffers(1, &uv_vbo));
  GL_CALL(glGenBuffers(1, &color_vbo));

  GL_CALL(glBindVertexArray(vao));
}

void draw(entt::registry &registry, entt::component component, Texture texture,
          glm::mat4 PVM, float magnification) {
  glm::vec4 p1 = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f};
  glm::vec4 p2 = glm::vec4{1.0f, 0.0f, 0.0f, 1.0f};
  glm::vec4 p3 = glm::vec4{0.0f, 1.0f, 0.0f, 1.0f};
  glm::vec4 p4 = glm::vec4{1.0f, 1.0f, 0.0f, 1.0f};

  entt::component components[] = {component, registry.type<CoItem>()};
  auto spriteView = registry.runtime_view(begin(components), end(components));
  if (spriteView.empty())
    return;

  vector<glm::vec2> screenPos_buf;
  vector<float> frame_buf;
  vector<glm::vec2> uv_buf;
  vector<int> color_buf;

  for (auto entity : spriteView) {
    auto item = registry.get<CoItem>(entity);

    glm::mat4 TM = glm::translate(PVM, glm::vec3(item.pos, 0.0f));
    TM = glm::rotate(TM, item.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    TM = glm::scale(TM,
                    glm::vec3(item.size.x, item.size.y, 1.0f) * magnification);
    TM = glm::translate(TM, glm::vec3(-0.5, -0.5, 0.0));

    glm::vec4 screenPos = TM * glm::vec4(0.5, 0.5, 0.0, 1.0);
    if (screenPos.x > -1.1 && screenPos.x < 1.1 && screenPos.y > -1.1 &&
        screenPos.y < 1.1) {

      screenPos_buf.push_back(glm::vec2(TM * p1));
      screenPos_buf.push_back(glm::vec2(TM * p2));
      screenPos_buf.push_back(glm::vec2(TM * p3));
      screenPos_buf.push_back(glm::vec2(TM * p2));
      screenPos_buf.push_back(glm::vec2(TM * p4));
      screenPos_buf.push_back(glm::vec2(TM * p3));

      float f = 0.0f;
      if (registry.has<CoAnimated>(entity)) {
        f = registry.get<CoAnimated>(entity).frame;
      }

      frame_buf.push_back(f);
      frame_buf.push_back(f);
      frame_buf.push_back(f);
      frame_buf.push_back(f);
      frame_buf.push_back(f);
      frame_buf.push_back(f);

      uv_buf.push_back(glm::vec2(p1));
      uv_buf.push_back(glm::vec2(p2));
      uv_buf.push_back(glm::vec2(p3));
      uv_buf.push_back(glm::vec2(p2));
      uv_buf.push_back(glm::vec2(p4));
      uv_buf.push_back(glm::vec2(p3));

      int p = -1;
      if (registry.has<CoPlayerAligned>(entity)) {
        p = registry.get<CoPlayer>(registry.get<CoPlayerAligned>(entity).player)
                .keySet;
      }
      color_buf.push_back(p);
      color_buf.push_back(p);
      color_buf.push_back(p);
      color_buf.push_back(p);
      color_buf.push_back(p);
      color_buf.push_back(p);
    }
  }

  GL_CALL(glUseProgram(item_shader));
  GL_CALL(glActiveTexture(GL_TEXTURE0));
  GL_CALL(glBindTexture(GL_TEXTURE_2D, texture.tex_id));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR_MIPMAP_LINEAR));
  GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GL_CALL(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -0.2));

  GL_CALL(glBindVertexArray(vao));

  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, position_vbo));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                       screenPos_buf.size() * sizeof(glm::vec2),
                       screenPos_buf.data(), GL_STREAM_DRAW));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, frame_vbo));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, frame_buf.size() * sizeof(GL_FLOAT),
                       frame_buf.data(), GL_STREAM_DRAW));
  GL_CALL(glEnableVertexAttribArray(1));
  GL_CALL(glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, uv_vbo));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, uv_buf.size() * sizeof(glm::vec2),
                       uv_buf.data(), GL_STREAM_DRAW));
  GL_CALL(glEnableVertexAttribArray(2));
  GL_CALL(glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, color_vbo));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, color_buf.size() * sizeof(int),
                       color_buf.data(), GL_STREAM_DRAW));
  GL_CALL(glEnableVertexAttribArray(3));
  GL_CALL(glVertexAttribIPointer(3, 1, GL_INT, 0, 0));

  GL_CALL(glUniform2i(item_shader_frameGridSize_uloc, texture.nx, texture.ny));

  GL_CALL(glDrawArrays(GL_TRIANGLES, 0, screenPos_buf.size()));
}

} // namespace DrawFloatingItems
