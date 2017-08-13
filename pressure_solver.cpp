#include "pressure_solver.hpp"

void gs(DoubleBuffered2DGrid& p, DoubleBuffered2DGrid& flag, int nx, int ny, float alpha) {
  for (int y = 1; y < ny - 1; y++) {
    for (int x = 1; x < nx - 1; x++) {
      p.f(x, y) = (1.0f - alpha) * p.f(x, y) +
                  alpha * ((p.f(x - 1, y) * flag.f(x - 1, y) +
                            p.f(x, y) * (1.0f - flag.f(x - 1, y))) +
                           (p.f(x + 1, y) * flag.f(x + 1, y) +
                            p.f(x, y) * (1.0f - flag.f(x + 1, y))) +
                           (p.f(x, y - 1) * flag.f(x, y - 1) +
                            p.f(x, y) * (1.0f - flag.f(x, y - 1))) +
                           (p.f(x, y + 1) * flag.f(x, y + 1) +
                            p.f(x, y) * (1.0f - flag.f(x, y + 1))) +
                           p.b(x, y)) *
                      0.25f;
    }
  }
}
void rbgs(DoubleBuffered2DGrid& p, DoubleBuffered2DGrid& flag, int nx, int ny) {

}
