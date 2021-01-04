#pragma once
#include "entt/entity/registry.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>
namespace DrawTracersCS {
void init();
// void playerTracersAdd(int pid, glm::vec2 pos);

void updatePlayerTracers(entt::registry &registry);
void drawPlayerTracers(glm::mat4 PVM);

void updateTracers(GLuint vel_tex, GLuint flag_tex, int nx, int ny, float dt,
                   float pwidth);
void draw(glm::mat4 PVM);
} // namespace DrawTracersCS
