#include "pressure_solver.hpp"
#include "draw_2dbuf.hpp"
#include <cmath>
#include <glm/gtx/transform.hpp>
#include <iostream>
#include <omp.h>

using namespace std;

inline float smoothingKernel(Single2DGrid &p, Single2DGrid &f, Single2DGrid &flag,
                      float h, float alpha, int x, int y) {
  float val = 0.0f;
  float sum = 0.0f;
  sum += flag(x - 1, y) + flag(x + 1, y) + flag(x, y + 1) + flag(x, y - 1);
  val += p(x - 1, y) * flag(x - 1, y); // + p(x, y) * (1.0f - flag(x - 1, y));
  val += p(x + 1, y) * flag(x + 1, y); // + p(x, y) * (1.0f - flag(x + 1, y));
  val += p(x, y - 1) * flag(x, y - 1); // + p(x, y) * (1.0f - flag(x, y - 1));
  val += p(x, y + 1) * flag(x, y + 1); // + p(x, y) * (1.0f - flag(x, y + 1));
  val += f(x, y) * h * h;
  val /= sum;
  if (sum == 0.0)
    val = 0;
  return flag(x, y) * (alpha * val + (1.0f - alpha) * p(x, y));
}

void gs(Single2DGrid &p, Single2DGrid &f, Single2DGrid &flag, float h,
        float alpha) {
  for (int y = 1; y < p.height - 1; y++) {
    for (int x = 1; x < p.width - 1; x++) {
      p(x, y) = smoothingKernel(p, f, flag, h, alpha, x, y);
    }
  }
}

inline void rbgs_red_line(int y, Single2DGrid &p, Single2DGrid &f, Single2DGrid &flag,
                   float h, float alpha) {
  for (int x = 1 + (y % 2); x < p.width - 1; x += 2) {
    p(x, y) = smoothingKernel(p, f, flag, h, alpha, x, y);
  }
}

inline void rbgs_black_line(int y, Single2DGrid &p, Single2DGrid &f,
                     Single2DGrid &flag, float h, float alpha) {
  for (int x = 1 + ((y + 1) % 2); x < p.width - 1; x += 2) {
    p(x, y) = smoothingKernel(p, f, flag, h, alpha, x, y);
  }
}

void rbgs(Single2DGrid &p, Single2DGrid &f, Single2DGrid &flag, float h,
          float alpha) {

  if (p.height < 20) {
    for (int y = 1; y < p.height - 1; y++)
      rbgs_red_line(y, p, f, flag, h, alpha);
    for (int y = 1; y < p.height - 1; y++)
      rbgs_black_line(y, p, f, flag, h, alpha);
    return;
  }

#pragma omp parallel
  {
    int num_threads = omp_get_num_threads();
    int thread_num = omp_get_thread_num();

    if (p.height / num_threads < 100) {
#pragma omp for
      for (int y = 1; y < p.height - 1; y++)
        rbgs_red_line(y, p, f, flag, h, alpha);
#pragma omp for
      for (int y = 1; y < p.height - 1; y++)
        rbgs_black_line(y, p, f, flag, h, alpha);
    } else {

      int start_line = 1 + (p.height - 2) * thread_num / num_threads;
      int end_line = 1 + (p.height - 2) * (thread_num + 1) / num_threads;

      rbgs_red_line(start_line, p, f, flag, h, alpha);
      rbgs_red_line(start_line+1, p, f, flag, h, alpha);

#pragma omp barrier

      for (int y = start_line+1; y < end_line - 1; y++) {
        rbgs_red_line(y + 1, p, f, flag, h, alpha);
        rbgs_black_line(y, p, f, flag, h, alpha);
      }

      rbgs_black_line(end_line-1, p, f, flag, h, alpha);
      rbgs_black_line(end_line, p, f, flag, h, alpha);
    }
  }
}

float calculateResidualField(Single2DGrid &p, Single2DGrid &f,
                             Single2DGrid &flag, Single2DGrid &r, float h) {
  float l2r = 0;

  float ihsq = 1.0f / h / h;

#pragma omp parallel for reduction(+ : l2r) if (p.height > 100)
  for (int y = 1; y < p.height - 1; y++) {
#pragma omp simd
    for (int x = 1; x < p.width - 1; x++) {
      float val = 0.0f;
      val += p(x - 1, y) * flag(x - 1, y) + p(x, y) * (1.0f - flag(x - 1, y));
      val += p(x + 1, y) * flag(x + 1, y) + p(x, y) * (1.0f - flag(x + 1, y));
      val += p(x, y - 1) * flag(x, y - 1) + p(x, y) * (1.0f - flag(x, y - 1));
      val += p(x, y + 1) * flag(x, y + 1) + p(x, y) * (1.0f - flag(x, y + 1));
      val += -4.0f * p(x, y);
      val *= ihsq;

      r(x, y) = (f(x, y) + val) * flag(x, y);

      l2r += r(x, y) * r(x, y);
    }
  }

  return sqrt(l2r);
}

void restrict(Single2DGrid &r, Single2DGrid &rc) {
#pragma omp parallel for if (rc.height > 30)
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

void prolongate(Single2DGrid &r, Single2DGrid &rc, Single2DGrid &flagc,
                Single2DGrid &flag) {
  r = 0.0;

#pragma omp parallel if (r.height > 5)
  {
#pragma omp for nowait
    for (int y = 2; y < r.height - 1; y += 2) {
      for (int x = 2; x < r.width - 1; x += 2) {
        r(x, y) = rc(x / 2, y / 2) * flag(x, y);
      }
    }
#pragma omp for nowait
    for (int y = 2; y < r.height - 1; y += 2) {
      for (int x = 1; x < r.width - 2; x += 2) {
        float sum = flagc(x / 2, y / 2) + flagc(x / 2 + 1, y / 2) + 0.0001;
        r(x, y) = flag(x, y) * (rc(x / 2, y / 2) + rc(x / 2 + 1, y / 2)) / sum;
      }
    }
#pragma omp for nowait
    for (int y = 1; y < r.height - 2; y += 2) {
      for (int x = 2; x < r.width - 1; x += 2) {
        float sum = flagc(x / 2, y / 2) + flagc(x / 2, y / 2 + 1) + 0.0001;
        r(x, y) = flag(x, y) * (rc(x / 2, y / 2) + rc(x / 2, y / 2 + 1)) / sum;
      }
    }
#pragma omp for nowait
    for (int y = 1; y < r.height - 2; y += 2) {
      for (int x = 1; x < r.width - 2; x += 2) {
        float sum = flagc(x / 2, y / 2) + flagc(x / 2 + 1, y / 2 + 1) +
                    flagc(x / 2 + 1, y / 2) + flagc(x / 2, y / 2 + 1) + 0.0001;
        r(x, y) = flag(x, y) *
                  (rc(x / 2, y / 2) + rc(x / 2 + 1, y / 2 + 1) +
                   rc(x / 2 + 1, y / 2) + rc(x / 2, y / 2 + 1)) /
                  sum;
      }
    }
  }
}

void correct(Single2DGrid &p, Single2DGrid &e) {
#pragma omp parallel for if (p.height > 30)
  for (int y = 1; y < p.height - 1; y++) {
    for (int x = 1; x < p.width - 1; x++) {
      p(x, y) += e(x, y) * 1.0;
    }
  }
}

void setZeroGradientBC(Single2DGrid &p) {
  for (int y = 1; y < p.height - 1; y++) {
    p(0, y) = p(1, y);
    p(p.width - 1, y) = p(p.width - 2, y);
  }
  for (int x = 1; x < p.width - 1; x++) {
    p(x, 0) = p(x, 1);
    p(x, p.height - 1) = p(x, p.height - 2);
  }
}

/*void drawGrid(Single2DGrid &grid) {
  glm::mat4 PVM(1.0f);
  PVM = glm::translate(PVM, glm::vec3(-1, -0.5, 0.0));
  PVM = glm::scale(PVM, glm::vec3(2.0, 2.0, 2.0));
  Draw2DBuf::draw_scalar(grid.data(), grid.width, grid.height, PVM, 1.0f);
  }*/

void MG::solveLevel(Single2DGrid &p, Single2DGrid &f, Single2DGrid &flag,
                    float h, int level, bool zeroGradientBC) {
  if (level == levels - 2) {
    for (int i = 0; i < 5; i++) {
      rbgs(p, f, flag, h, 1.0);
    }
    // drawGrid(flag);
    return;
  }

  for (int i = 0; i < 3; i++) {
    rbgs(p, f, flag, h, 1.0);
    if (level == 0 && zeroGradientBC)
      setZeroGradientBC(p);
  }

  auto &r = rs[level];
  r = 0.0;
  calculateResidualField(p, f, flag, r, h);

  auto &rc = rcs[level + 1];
  rc = 0.0;
  restrict(r, rc);

  auto &ec = ecs[level + 1];
  ec = 0.0;
  auto &flagc = flagcs[level + 1];

  MG::solveLevel(ec, rc, flagc, h * (r.width - 1.0f) / (rc.width - 1.0f),
                 level + 1);

  auto &e = es[level];

  prolongate(e, ec, flagc, flag);

  //  maskFlag(e, flag);

  correct(p, e);

  if (level == 0 && zeroGradientBC)
    setZeroGradientBC(p);

  for (int i = 0; i < 3; i++) {
    rbgs(p, f, flag, h, 1.0);
    if (level == 0 && zeroGradientBC)
      setZeroGradientBC(p);
  }
};
