#pragma once
#include <glm/vec2.hpp>

namespace DrawTracers {
void init();
void draw(float* bufx, float* bufy, float* bufFlag, int nx, int ny,
          int screen_width, int screen_height, float scale, glm::vec2 translate,
          float dt, float h);
}
