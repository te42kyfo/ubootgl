#include "draw_floating_items.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <vector>
#include "calculate_scale_factors.hpp"
#include "gl_error.hpp"
#include "load_shader.hpp"

namespace DrawFloatingItems {

using namespace std;

GLuint item_shader;
GLuint vao, quad_vbo;
GLint item_shader_TM_uloc;

void init() {
  item_shader = loadShader("./scale_translate2D.vert", "./white.frag",
                           {{0, "in_Position"}, {1, "in_Alpha"}});
  item_shader_TM_uloc = glGetUniformLocation(item_shader, "TM");

  GL_CALL(glGenVertexArrays(1, &vao));

  // Quad Geometry
  GL_CALL(glGenVertexArrays(1, &vao));
  GL_CALL(glGenBuffers(1, &quad_vbo));

  float vertex_data[4][4] = {{-1.0f, -1.0f, 0.0f, 1.0f},
                             {1.0f, -1.0f, 0.0f, 1.0f},
                             {-1.0f, 1.0f, 0.0f, 1.0f},
                             {1.0f, 1.0f, 0.0f, 1.0f}};

  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, quad_vbo));
  GL_CALL(glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(GLfloat), vertex_data,
                       GL_STATIC_DRAW));
}

void draw(std::vector<FloatingItem>& items, glm::mat4 PVM) {
  // Draw Quad with texture
  GL_CALL(glBindVertexArray(vao));
  GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, quad_vbo));
  GL_CALL(glEnableVertexAttribArray(0));
  GL_CALL(glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0));
  GL_CALL(glUseProgram(item_shader));

  for (const auto& item : items) {
    glm::mat4 TM = glm::translate(PVM, glm::vec3(item.pos, 0.0f));
    TM = glm::rotate(TM, item.rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    TM = glm::scale(TM, glm::vec3(item.size.x, item.size.y, 1.0f));
    TM = glm::scale(TM, glm::vec3(0.5f, 0.5f, 1.0f));

    GL_CALL(glUniformMatrix4fv(item_shader_TM_uloc, 1, GL_FALSE,
                               glm::value_ptr(TM)));

    GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
  }
}
}
