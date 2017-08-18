#include "pressure_solver.hpp"
#include <cmath>
#include <iostream>
#include "draw_2dbuf.hpp"

using namespace std;

void gs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
        float alpha) {
  for (int y = 1; y < p.height - 1; y++) {
    for (int x = 1; x < p.width - 1; x++) {
      float val = 0.0f;
      val += p(x - 1, y) * flag(x - 1, y) + p(x, y) * (1.0f - flag(x - 1, y));
      val += p(x + 1, y) * flag(x + 1, y) + p(x, y) * (1.0f - flag(x + 1, y));
      val += p(x, y - 1) * flag(x, y - 1) + p(x, y) * (1.0f - flag(x, y - 1));
      val += p(x, y + 1) * flag(x, y + 1) + p(x, y) * (1.0f - flag(x, y + 1));
      val += f(x, y) * h * h;
      p(x, y) = alpha * val * 0.25f + (1.0 - alpha) * p(x, y);
    }
  }
}
void rbgs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
          float alpha) {
#pragma omp parallel for
  for (int y = 1; y < p.height - 1; y++) {
    for (int x = 1 + (y % 2); x < p.width - 1; x += 2) {
      float val = 0.0f;
      val += p(x - 1, y) * flag(x - 1, y) + p(x, y) * (1.0f - flag(x - 1, y));
      val += p(x + 1, y) * flag(x + 1, y) + p(x, y) * (1.0f - flag(x + 1, y));
      val += p(x, y - 1) * flag(x, y - 1) + p(x, y) * (1.0f - flag(x, y - 1));
      val += p(x, y + 1) * flag(x, y + 1) + p(x, y) * (1.0f - flag(x, y + 1));
      val += f(x, y) * h * h;
      p(x, y) = alpha * val * 0.25f + (1.0 - alpha) * p(x, y);
    }
  }
#pragma omp parallel for
  for (int y = 1; y < p.height - 1; y++) {
    for (int x = 1 + ((y + 1) % 2); x < p.width - 1; x += 2) {
      float val = 0.0f;
      val += p(x - 1, y) * flag(x - 1, y) + p(x, y) * (1.0f - flag(x - 1, y));
      val += p(x + 1, y) * flag(x + 1, y) + p(x, y) * (1.0f - flag(x + 1, y));
      val += p(x, y - 1) * flag(x, y - 1) + p(x, y) * (1.0f - flag(x, y - 1));
      val += p(x, y + 1) * flag(x, y + 1) + p(x, y) * (1.0f - flag(x, y + 1));
      val += f(x, y) * h * h;
      p(x, y) = alpha * val * 0.25f + (1.0 - alpha) * p(x, y);
    }
  }
}

float calculateResidualField(Single2DGrid& p, Single2DGrid& f,
                             Single2DGrid& flag, Single2DGrid& r, float h) {
  float l2r = 0;

  float ihsq = 1.0f / h / h;
#pragma omp parallel for
  for (int y = 1; y < p.height - 1; y++) {
    for (int x = 1; x < p.width - 1; x++) {
      float val = 0.0f;
      val +=
          p(x - 1, y);  // * flag(x - 1, y) + p(x, y) * (1.0f - flag(x - 1, y));
      val +=
          p(x + 1, y);  // * flag(x + 1, y) + p(x, y) * (1.0f - flag(x + 1, y));
      val +=
          p(x, y - 1);  // * flag(x, y - 1) + p(x, y) * (1.0f - flag(x, y - 1));
      val +=
          p(x, y + 1);  // * flag(x, y + 1) + p(x, y) * (1.0f - flag(x, y + 1));
      val += -4.0 * p(x, y);
      val *= ihsq;

      r(x, y) = f(x, y) + val;

      l2r += r(x, y) * r(x, y) * flag(x, y);
    }
  }

  return sqrt(l2r);
}

void restrict(Single2DGrid& r, Single2DGrid& rc) {
#pragma omp parallel for
  for (int y = 1; y < rc.height - 1; y++) {
    for (int x = 1; x < rc.width - 1; x++) {
      float v = 0.0;
      v = r(2 * x - 1, 2 * y - 1) * 1 + r(2 * x + 0, 2 * y - 1) * 2 +
          r(2 * x + 1, 2 * y - 1) * 1;
      v += r(2 * x - 1, 2 * y + 0) * 2 + r(2 * x + 0, 2 * y + 0) * 4 +
           r(2 * x + 1, 2 * y + 0) * 2;
      v += r(2 * x - 1, 2 * y + 1) * 1 + r(2 * x + 0, 2 * y + 1) * 2 +
           r(2 * x + 1, 2 * y + 1) * 1;
      rc(x, y) = v * (1.0f / 16.0f);
    }
  }
}

void prolongate(Single2DGrid& r, Single2DGrid& rc) {
  r = 0.0;
  for (int y = 1; y < rc.height - 1; y++) {
    for (int x = 1; x < rc.width - 1; x++) {
      r(2 * x - 1, 2 * y - 1) += rc(x, y) * (1 / 4.0f);
      r(2 * x + 0, 2 * y - 1) += rc(x, y) * (2 / 4.0f);
      r(2 * x + 1, 2 * y - 1) += rc(x, y) * (1 / 4.0f);
      r(2 * x - 1, 2 * y + 0) += rc(x, y) * (2 / 4.0f);
      r(2 * x + 0, 2 * y + 0) += rc(x, y) * (4 / 4.0f);
      r(2 * x + 1, 2 * y + 0) += rc(x, y) * (2 / 4.0f);
      r(2 * x - 1, 2 * y + 1) += rc(x, y) * (1 / 4.0f);
      r(2 * x + 0, 2 * y + 1) += rc(x, y) * (2 / 4.0f);
      r(2 * x + 1, 2 * y + 1) += rc(x, y) * (1 / 4.0f);
    }
  }
}

void correct(Single2DGrid& p, Single2DGrid& e) {
#pragma omp parallel for
  for (int y = 1; y < p.height - 1; y++) {
    for (int x = 1; x < p.width - 1; x++) {
      p(x, y) += e(x, y) * 1.0;
    }
  }
}

void setZeroGradientBC(Single2DGrid& p) {
  for (int y = 1; y < p.height - 1; y++) {
    p(0, y) = p(1, y);
    p(p.width - 1, y) = p(p.width - 2, y);
  }
  for (int x = 1; x < p.width - 1; x++) {
    p(x, 0) = p(x, 1);
    p(x, p.height - 1) = p(x, p.height - 2);
  }
}

void MG::solveLevel(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag,
                    float h, int level, bool zeroGradientBC) {

  if (level == levels - 1) {
    for (int i = 0; i < 1; i++) {
      rbgs(p, f, flag, h, 1.0);
    }
    return;
  }

  for (int i = 0; i < 2; i++) {
    rbgs(p, f, flag, h, 1.0);
    if (level == 0 && zeroGradientBC) setZeroGradientBC(p);
  }

  auto& r = rs[level];
  r = 0.0;
  calculateResidualField(p, f, flag, r, h);

  auto& rc = rcs[level + 1];
  rc = 0.0;
  restrict(r, rc);

  auto& ec = ecs[level + 1];
  ec = 0.0;
  auto& flagc = flagcs[level + 1];
  flagc = 1.0;

  MG::solveLevel(ec, rc, flagc, h * (r.width - 1.0f) / (rc.width - 1.0f),
                 level + 1);

  auto& e = es[level];
  prolongate(e, ec);

  correct(p, e);
  if (level == 0 && zeroGradientBC) setZeroGradientBC(p);

  for (int i = 0; i < 2; i++) {
    rbgs(p, f, flag, h, 1.0);
    if (level == 0 && zeroGradientBC) setZeroGradientBC(p);
  }
};
