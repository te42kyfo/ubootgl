#include <glm/vec2.hpp>

namespace DrawStreamlines{
void init();
void draw(float* bufx, float* bufy, int nx, int ny, int screen_width,
          int screen_height, float scale, glm::vec2 translate);
}
