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
  for (int y = 1; y < p.width - 1; y++) {
    p(0, y) = p(1, y);
    p(p.width - 1, y) = p(p.width - 2, y);
  }
  for (int x = 1; x < p.height - 1; x++) {
    p(x, 0) = p(x, p.height - 1);
    p(x, p.height - 1) = p(x, p.height - 2);
  }
}

void mg(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
        bool zeroGradientBC) {
  if (p.width <= 5 || p.height <= 5) {
    for (int i = 0; i < 20; i++) {
      rbgs(p, f, flag, h, 1.0);
    }
    return;
  }

  for (int i = 0; i < 2; i++) {
    rbgs(p, f, flag, h, 1.5);
    if (zeroGradientBC) setZeroGradientBC(p);
  }

  Single2DGrid r(p.width, p.height);
  r = 0.0;
  calculateResidualField(p, f, flag, r, h);

  int cx = (int)((p.width) / 2.0);
  int cy = (int)((p.height) / 2);

  Single2DGrid rc(cx, cy);
  rc = 0.0;
  Single2DGrid ec(cx, cy);
  ec = 0.0;
  Single2DGrid flagc(cx, cy);
  flagc = 1.0;
  restrict(r, rc);
  //  r.print();
  // rc.print();

  mg(ec, rc, flagc, h * (r.width - 1.0f) / (rc.width - 1.0f));
  // cout << "\n";
  //  ec.print();

  Single2DGrid e(p.width, p.height);
  prolongate(e, ec);
  // e.print();
  correct(p, e);

  if (zeroGradientBC) setZeroGradientBC(p);

  for (int i = 0; i < 2; i++) {
    rbgs(p, f, flag, h, 1.3);
    if (zeroGradientBC) setZeroGradientBC(p);
  }
}
