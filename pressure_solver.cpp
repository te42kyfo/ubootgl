#include "pressure_solver.hpp"
#include <cmath>

using namespace std;

void gs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, int nx, int ny,
        float h, float alpha) {
  for (int y = 1; y < ny - 1; y++) {
    for (int x = 1; x < nx - 1; x++) {
      p(x, y) = (1.0f - alpha) * p(x, y) +
                alpha * ((p(x - 1, y) * flag(x - 1, y) +
                          p(x, y) * (1.0f - flag(x - 1, y))) +
                         (p(x + 1, y) * flag(x + 1, y) +
                          p(x, y) * (1.0f - flag(x + 1, y))) +
                         (p(x, y - 1) * flag(x, y - 1) +
                          p(x, y) * (1.0f - flag(x, y - 1))) +
                         (p(x, y + 1) * flag(x, y + 1) +
                          p(x, y) * (1.0f - flag(x, y + 1))) -
                         f(x, y) * h * h) *
                    0.25f;
    }
  }
}
void rbgs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, int nx, int ny,
          float h, float alpha) {
#pragma omp parallel for
  for (int y = 1; y < ny - 1; y++) {
    for (int x = 1 + (y % 2); x < nx - 1; x += 2) {
      p(x, y) = (1.0f - alpha) * p(x, y) +
                alpha * ((p(x - 1, y) * flag(x - 1, y) +
                          p(x, y) * (1.0f - flag(x - 1, y))) +
                         (p(x + 1, y) * flag(x + 1, y) +
                          p(x, y) * (1.0f - flag(x + 1, y))) +
                         (p(x, y - 1) * flag(x, y - 1) +
                          p(x, y) * (1.0f - flag(x, y - 1))) +
                         (p(x, y + 1) * flag(x, y + 1) +
                          p(x, y) * (1.0f - flag(x, y + 1))) -
                         f(x, y) * h * h) *
                    0.25f;
    }
  }
#pragma omp parallel for
  for (int y = 1; y < ny - 1; y++) {
    for (int x = 1 + ((y + 1) % 2); x < nx - 1; x += 2) {
      p(x, y) = (1.0f - alpha) * p(x, y) +
                alpha * ((p(x - 1, y) * flag(x - 1, y) +
                          p(x, y) * (1.0f - flag(x - 1, y))) +
                         (p(x + 1, y) * flag(x + 1, y) +
                          p(x, y) * (1.0f - flag(x + 1, y))) +
                         (p(x, y - 1) * flag(x, y - 1) +
                          p(x, y) * (1.0f - flag(x, y - 1))) +
                         (p(x, y + 1) * flag(x, y + 1) +
                          p(x, y) * (1.0f - flag(x, y + 1))) -
                         f(x, y) * h * h) *
                    0.25f;
    }
  }
}

float calculateResidualField(Single2DGrid& p, Single2DGrid& f,
                            Single2DGrid& flag, Single2DGrid& r, int nx, int ny,
                            float h) {
  float l2r = 0;

  for (int y = 1; y < ny - 1; y++) {
    for (int x = 1; x < nx - 1; x++) {
      r(x, y) = 0.25f * ((p(x - 1, y) * flag(x - 1, y) +
                          p(x, y) * (1.0f - flag(x - 1, y))) +
                         (p(x + 1, y) * flag(x + 1, y) +
                          p(x, y) * (1.0f - flag(x + 1, y))) +
                         (p(x, y - 1) * flag(x, y - 1) +
                          p(x, y) * (1.0f - flag(x, y - 1))) +
                         (p(x, y + 1) * flag(x, y + 1) +
                          p(x, y) * (1.0f - flag(x, y + 1))) -
                         f(x, y) * h * h) -
                p(x, y);

      l2r += r(x, y) * r(x, y) * flag(x, y);
    }
  }

  return sqrt(l2r);
}
