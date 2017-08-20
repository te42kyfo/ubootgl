#include "pressure_solver.hpp"
#include <cmath>
#include <iostream>
#include "draw_2dbuf.hpp"

using namespace std;

float smoothingKernel(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag,
                      float h, float alpha, int x, int y) {
  float val = 0.0f;
  float sum = 0.0f;
  sum += flag(x - 1, y) + flag(x + 1, y) + flag(x, y + 1) + flag(x, y - 1);
  val += p(x - 1, y) * flag(x - 1, y);  // + p(x, y) * (1.0f - flag(x - 1, y));
  val += p(x + 1, y) * flag(x + 1, y);  // + p(x, y) * (1.0f - flag(x + 1, y));
  val += p(x, y - 1) * flag(x, y - 1);  // + p(x, y) * (1.0f - flag(x, y - 1));
  val += p(x, y + 1) * flag(x, y + 1);  // + p(x, y) * (1.0f - flag(x, y + 1));
  val += f(x, y) * h * h;
  val /= sum;
  if (sum == 0.0) val = 0;
  return flag(x, y) * val;  //(alpha * val + (1.0 - alpha) * p(x, y));
}

void gs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
        float alpha) {
  for (int y = 1; y < p.height - 1; y++) {
    for (int x = 1; x < p.width - 1; x++) {
      p(x, y) = smoothingKernel(p, f, flag, h, alpha, x, y);
    }
  }
}
void rbgs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
          float alpha) {
#pragma omp parallel for
  for (int y = 1; y < p.height - 1; y++) {
    for (int x = 1 + (y % 2); x < p.width - 1; x += 2) {
      p(x, y) = smoothingKernel(p, f, flag, h, alpha, x, y);
    }
  }
#pragma omp parallel for
  for (int y = 1; y < p.height - 1; y++) {
    for (int x = 1 + ((y + 1) % 2); x < p.width - 1; x += 2) {
      p(x, y) = smoothingKernel(p, f, flag, h, alpha, x, y);
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
      val += p(x - 1, y) * flag(x - 1, y) + p(x, y) * (1.0f - flag(x - 1, y));
      val += p(x + 1, y) * flag(x + 1, y) + p(x, y) * (1.0f - flag(x + 1, y));
      val += p(x, y - 1) * flag(x, y - 1) + p(x, y) * (1.0f - flag(x, y - 1));
      val += p(x, y + 1) * flag(x, y + 1) + p(x, y) * (1.0f - flag(x, y + 1));
      val += -4.0 * p(x, y);
      val *= ihsq;

      r(x, y) = (f(x, y) + val) * flag(x, y);

      l2r += r(x, y) * r(x, y);
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

void prolongate(Single2DGrid& r, Single2DGrid& rc, Single2DGrid& flagc) {
  r = 0.0;
  for (int y = 2; y < r.height - 1; y += 2) {
    for (int x = 2; x < r.width - 1; x += 2) {
      r(x, y) = rc(x / 2, y / 2);
    }
  }
  for (int y = 2; y < r.height - 1; y += 2) {
    for (int x = 1; x < r.width - 2; x += 2) {
      float sum = flagc(x / 2, y / 2) + flagc(x / 2 + 1, y / 2) + 0.0001;
      r(x, y) = (rc(x / 2, y / 2) + rc(x / 2 + 1, y / 2)) / sum;
    }
  }
  for (int y = 1; y < r.height - 2; y += 2) {
    for (int x = 2; x < r.width - 1; x += 2) {
      float sum = flagc(x / 2, y / 2) + flagc(x / 2, y / 2 + 1) + 0.0001;
      r(x, y) = (rc(x / 2, y / 2) + rc(x / 2, y / 2 + 1)) / sum;
    }
  }
  for (int y = 1; y < r.height - 2; y += 2) {
    for (int x = 1; x < r.width - 2; x += 2) {
      float sum = flagc(x / 2, y / 2) + flagc(x / 2 + 1, y / 2 + 1) +
                  flagc(x / 2 + 1, y / 2) + flagc(x / 2, y / 2 + 1) + 0.0001;
      r(x, y) = (rc(x / 2, y / 2) + rc(x / 2 + 1, y / 2 + 1) +
                 rc(x / 2 + 1, y / 2) + rc(x / 2, y / 2 + 1)) /
                sum;
    }
  }

  /*  for (int y = 1; y < rc.height - 1; y++) {
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
    }*/
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

void maskFlag(Single2DGrid& p, Single2DGrid& flag) {
  for (int y = 0; y < p.height; y++) {
    for (int x = 0; x < p.width; x++) {
      p(x, y) *= flag(x, y);
    }
  }
}

void drawGrid(Single2DGrid& grid) {
  Draw2DBuf::draw_scalar(grid.data(), grid.width, grid.height, 1600, 900, 1.00, 0,
                         0);
}

void MG::solveLevel(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag,
                    float h, int level, bool zeroGradientBC) {
  if (level == 1) {
    for (int i = 0; i < 10; i++) {
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
  if (level == 0) drawGrid(rc);
  auto& ec = ecs[level + 1];
  ec = 0.0;
  auto& flagc = flagcs[level + 1];

  MG::solveLevel(ec, rc, flagc, h * (r.width - 1.0f) / (rc.width - 1.0f),
                 level + 1);

  auto& e = es[level];

  prolongate(e, ec, flagc);

  maskFlag(e, flag);

    correct(p, e);

  if (level == 0 && zeroGradientBC) setZeroGradientBC(p);

  for (int i = 0; i < 0; i++) {
    rbgs(p, f, flag, h, 1.0);
    if (level == 0 && zeroGradientBC) setZeroGradientBC(p);
  }
};
