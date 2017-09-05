#include <glm/glm.hpp>

namespace DrawStreamlines {
void init();
void draw(float* bufx, float* bufy, int nx, int ny, glm::mat4 PVM);
}
