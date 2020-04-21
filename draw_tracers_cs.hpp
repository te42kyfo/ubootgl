#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
namespace DrawTracersCS {
void init();
// void playerTracersAdd(int pid, glm::vec2 pos);

void updateTracers(GLuint vel_tex, GLuint flag_tex, int nx, int ny, float dt,
                   float pwidth);

void draw(int nx, int ny, glm::mat4 PVM, float pwidth);
} // namespace DrawTracersCS
