#pragma once
#include <functional>
#include <iostream>
#include <vector>

class DoubleBuffered2DGrid {
 public:
  DoubleBuffered2DGrid(int width, int height)
      : width(width),
        height(height),
        front(0),
        back(1),
        v{std::vector<float>(width * height),
          std::vector<float>(width * height)} {}

  int idx(int x, int y) { return y * width + x; }
  void swap() { std::swap(front, back); }
  float& f(int x, int y) { return v[front][idx(x, y)]; }
  float& b(int x, int y) { return v[back][idx(x, y)]; }
  float* data() { return v[front].data(); }
  void copyFrontToBack() {
    for (int i = 0; i < width * height; i++) {
      v[back][i] = v[front][i];
    }
  }

 private:
  const int width, height;
  int front, back;
  std::vector<float> v[2];
};

class Simulation {
 public:
  Simulation(float pwidth, float mu, int width, int height)
      : pwidth(pwidth),
        mu(mu),
        width(width),
        height(height),
        vx(width, height),
        vy(width, height),
        p(width, height) {
    for (int x = 10; x < width * 0.9; x++) {
      vx.f(x, 500) = 0.2;
      vx.f(500, x) = -0.2;
    }

    dt = pwidth / width / 0.01 * 0.5;
    std::cout << "SIM: dt=" << dt << "\n";
  }

  float* getVX() { return vx.data(); }
  float* getVY() { return vy.data(); }
  float* getP() { return p.data(); }

  float diffusion_l2_residual(DoubleBuffered2DGrid& v) {
    float a = dt * mu * (pwidth / width);
    float residual = 0.0;
    for (int y = 1; y < height - 1; y++) {
      for (int x = 1; x < width - 1; x++) {
        float lres = (v.f(x, y) +
                      a * (v.b(x + 1, y) + v.b(x - 1, y) + v.b(x, y + 1) +
                           v.b(x, y - 1))) /
                         (1.0f + 4.0f * a) -
                     v.b(x, y);
        residual += lres * lres;
      }
    }
    return sqrt(residual) / width / height;
  }

  void diffuse(DoubleBuffered2DGrid& v) {
    // use this timestep as initial guess
    v.copyFrontToBack();
    float a = dt * mu * (pwidth / width);
    float omega = 1.01;
    for (int i = 1; true; i++) {
      for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
          v.b(x, y) = v.b(x, y) * (1.0f - omega) +
                      omega * (v.f(x, y) +
                               a * (v.b(x + 1, y) + v.b(x - 1, y) +
                                    v.b(x, y + 1) + v.b(x, y - 1))) /
                          (1.0f + 4.0f * a);
        }
      }
      if (i % 1 == 0) {
        float res = diffusion_l2_residual(v);
        if (res < 1.0e-6 || i >= 100) {
          std::cout << std::setprecision(4) << std::scientific
                    << "SIM DIFFUSION:" << i << " iters, res=" << res << "\n";
          break;
        }
      }
    }
    v.swap();
  }

  void step() {
    for (int i = 0; i < 10000; i++) {
      int rx = 1 + rand() % (width - 2);
      int ry = 1 + rand() % (height - 2);

      vx.f(rx, ry) += (i % 100 == 0 ? -99 : 1);
    }
    diffuse(vx);
    diffuse(vy);
  }

  float pwidth;
  float mu;
  const int width, height;
  float dt;

 private:
  DoubleBuffered2DGrid vx, vy, p;
};
