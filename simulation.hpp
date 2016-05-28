#pragma once
#include <iostream>
#include <vector>

class Simulation {
 public:
  Simulation(float pwidth, float mu, int width, int height)
      : pwidth(pwidth),
        mu(mu),
        width(width),
        height(height),
        vx{std::vector<float>(width * height),
           std::vector<float>(width * height)},
        vy{std::vector<float>(width * height),
           std::vector<float>(width * height)},
        p{std::vector<float>(width * height),
          std::vector<float>(width * height)},
        front(0),
        back(1) {
    for (int x = 10; x < width * 0.9; x++) {
      vxf(x, 500) = 0.2;
      vxf(500, x) = -0.2;
    }

    dt = pwidth / width / 0.1f * 0.5;
    std::cout << "SIM: dt=" << dt << "\n";
  }

  float* getVX() { return vx[front].data(); }
  float* getVY() { return vy[front].data(); }
  float* getP() { return p[front].data(); }

  int idx(int x, int y) { return y * width + x; }

  float& vxf(int x, int y) { return vx[front][idx(x, y)]; }
  float& vxb(int x, int y) { return vx[back][idx(x, y)]; }

  float diffusion_residual() {
    float a = dt * mu * (pwidth / width);
    float residual = 0.0;
    for (int y = 1; y < height - 1; y++) {
      for (int x = 1; x < width - 1; x++) {
        float lres = (vxf(x, y) +
                      a * (vxb(x + 1, y) + vxb(x - 1, y) + vxb(x, y + 1) +
                           vxb(x, y - 1))) /
                         (1.0f + 4.0f * a) -
                     vxb(x, y);
        residual += lres * lres;
      }
    }
    return sqrt(residual);
  }

  void diffuse() {
    // use this timestep as initial guess
    for (int y = 1; y < height - 1; y++) {
      for (int x = 1; x < width - 1; x++) {
        vxb(x, y) = vxf(x, y);
      }
    }
    float a = dt * mu * (pwidth / width);
    for (int i = 0; i < 1000; i++) {
      for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
          vxb(x, y) = (vxf(x, y) +
                       a * (vxb(x + 1, y) + vxb(x - 1, y) + vxb(x, y + 1) +
                            vxb(x, y - 1))) /
                      (1.0f + 4.0f * a);
        }
      }
      if (i % 2 == 0) {
        float res = diffusion_residual();
        if (res < 1.0e-1) {
          std::cout << std::setprecision(4) << std::scientific
                    << "SIM DIFFUSION:" << i << " iters, res=" << res << "\n";
          break;
        }
      }
    }

    std::swap(front, back);
  }

  void step() {
    for (int i = 0; i < 1000; i++) {
      int rx = 1 + rand() % (width - 2);
      int ry = 1 + rand() % (height - 2);
      vxf(rx, ry) = (i % 3) - 1.0;
    }
    diffuse();
  }

  float pwidth;
  float mu;
  const int width, height;
  float dt;

 private:
  std::vector<float> vx[2], vy[2], p[2];
  int front;
  int back;
};
