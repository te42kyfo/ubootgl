#include <GL/glew.h>

namespace Draw2DBuf {
void init();
void draw(float* buf, size_t nx, size_t ny, int pixel_width, int pixel_height);
}
