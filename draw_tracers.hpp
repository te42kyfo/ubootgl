#pragma once
#include <glm/glm.hpp>

namespace DrawTracers {
void init();
void playerTracersAdd(int pid, glm::vec2 pos);

void updateTracers(float *vx, float *vy, float *flag, int nx, int ny, float dt,
                   float pwidth);
    
void draw(int nx, int ny, glm::mat4 PVM, float pwidth);
} // namespace DrawTracers
