#pragma once
#include <glm/glm.hpp>

namespace DrawTracers {
void init();
void draw(float* bufx, float* bufy, float* bufFlag, int nx, int ny,
          glm::mat4 PVM, float dt, float h);
}
