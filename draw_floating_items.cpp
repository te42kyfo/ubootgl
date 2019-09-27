#include "draw_floating_items.hpp"
#include "gl_error.hpp"
#include "load_shader.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <vector>

namespace DrawFloatingItems {

using namespace std;

GLuint item_shader;
GLuint vao, quad_vbo;
GLint item_shader_TM_uloc, item_shader_frameGridSize_uloc,
    item_shader_frame_uloc;

void init() {
  item_shader = loadShader("./scale_translate2D.vert", "./plain_tex.frag",
                           {{0, "in_Position"}});
  item_shader_TM_uloc = glGetUniformLocation(item_shader, "TM");

  item_shader_frameGridSize_uloc =
      glGetUniformLocation(item_shader, "frameGridSize");

  item_shader_frame_uloc = glGetUniformLocation(item_shader, "frame");

  GL_CALL(glGenVertexArrays(1, &vao));

  // Quad Geometry
  GL_CALL(glGenVertexArrays(1, &vao));
  GL_CALL(glGenBuffers(1, &quad_vbo));

  float vertex_data[4][4] = {{0.0f, 0.0f, 0.0f, 1.0f},
                             {1.0f, 0.0f, 0.0f, 1.0f},
                             {0.0f, 1.0f, 0.0f, 1.0f},
                             {1.0f, 1.0f, 0.0f, 1.0f}};

  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, quad_vbo));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), vertex_data,
                       GL_STATIC_DRAW));
}

template <typename T> void draw(T *begin, T *end, glm::mat4 PVM) {
  GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  // Draw Quad with texture
  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, quad_vbo));

  GL_CALL(glEnableVertexAttribArray(0));

  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));

  GL_CALL(glUseProgram(item_shader));

  for (auto it = begin; it != end; it++) {
    auto &item = *it;
    GL_CALL(glActiveTexture(GL_TEXTURE0));

    GL_CALL(glBindTexture(GL_TEXTURE_2D, item.tex->tex_id));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR_MIPMAP_LINEAR));

    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

    glm::mat4 TM = glm::translate(PVM, glm::vec3(item.pos, 0.0f));
    TM = glm::rotate(TM, item.rotation - glm::half_pi<float>(),
                     glm::vec3(0.0f, 0.0f, 1.0f));
    TM = glm::scale(TM, glm::vec3(item.size.x, item.size.y, 1.0f));
    TM = glm::translate(TM, glm::vec3(-0.5, -0.5, 0.0));

    glm::vec4 screenPos = TM * glm::vec4(0.5, 0.5, 0.0, 1.0);
    if (screenPos.x < -1.1 || screenPos.x > 1.1 || screenPos.y < -1.1 ||
        screenPos.y > 1.1)
      continue;

    GL_CALL(glUniformMatrix4fv(item_shader_TM_uloc, 1, GL_FALSE,
                               glm::value_ptr(TM)));

    GL_CALL(glUniform1f(item_shader_frame_uloc, item.frame));
    GL_CALL(glUniform2i(item_shader_frameGridSize_uloc, item.tex->nx,
                        item.tex->ny));

    GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
}

template void draw<FloatingItem>(FloatingItem *begin, FloatingItem *end,
                                 glm::mat4 PVM);
template void draw<Torpedo>(Torpedo *begin, Torpedo *end, glm::mat4 PVM);
} // namespace DrawFloatingItems
