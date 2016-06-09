#pragma once
#include <cmath>
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
    /*    for (int x = 0; x < width; x++) {
      vy.f(x, 0) = x % 2 * -100;
      vy.b(x, 0) = x % 2 * -100;
      }*/
    //   for (int y = 0; y < height; y++) {
    vx.f(0, 65) = 100;
    vx.b(0, 65) = 100;
    //}
    /*    for (int y = 0.498 * height; y < height * 0.502; y++) {
   vx.f(0, y) = 100.0;
   vx.b(0, y) = 100.0;
   }*/
  }

  float* getVX() { return vx.data(); }
  float* getVY() { return vy.data(); }
  float* getP() { return p.data(); }

  float diffusion_l2_residual(DoubleBuffered2DGrid& v) {
    float a = dt * mu * pwidth / (width - 1.0f);
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
    float a = dt * mu * pwidth / (width - 1.0f);
    float omega = 1.0f;
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
      if ((i - 1) % 2 == 0) {
        float res = diffusion_l2_residual(v);
        if (res < 1.0e-9 || i >= 100) {
          std::cout << std::setprecision(3) << std::scientific
                    << "SIM DIFFUSION: " << i << " iterations, res=" << res
                    << "\n";
          break;
        }
      }
    }
    v.swap();
  }

  float projection_residual() {
    float residual = 0;
    for (int y = 1; y < height - 1; y++) {
      for (int x = 1; x < width - 1; x++) {
        float local_residual = (p.b(x, y) + p.f(x - 1, y) + p.f(x + 1, y) +
                                p.f(x, y + 1) + p.f(x, y - 1)) *
                                   0.25f -
                               p.f(x, y);
        residual += local_residual * local_residual;
      }
    }
    return sqrt(residual) / width / height;
  }
  void project() {
    float h = pwidth / (width - 1.0f);
    float ih = 1.0f / h;
    float previous_residual = -1;
    for (int y = 1; y < height - 1; y++) {
      for (int x = 1; x < width - 1; x++) {
        p.b(x, y) = -0.5f * h * (vx.f(x + 1, y) - vx.f(x - 1, y) +
                                 vy.f(x, y + 1) - vy.f(x, y - 1));
      }
    }
    float alpha = 1.6;
    for (int i = 1; true; i++) {
      for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
          p.f(x, y) = (1.0f - alpha) * p.f(x, y) +
                      alpha * (p.b(x, y) + p.f(x - 1, y) + p.f(x + 1, y) +
                               p.f(x, y + 1) + p.f(x, y - 1)) *
                          0.25f;
        }
      }
      for (int y = 1; y < height - 1; y++) {
        p.f(0, y) = p.f(1, y);
        p.f(width - 1, y) = p.f(width - 2, y);
      }
      for (int x = 1; x < width - 1; x++) {
        p.f(x, 0) = p.f(x, 1);
        p.f(x, height - 1) = p.f(x, height - 2);
      }
      if ((i - 1) % 2 == 0) {
        float residual = projection_residual();
        if ((i > 1 && previous_residual / residual < 1.05f) ||
            residual < 1.0e-9 || i >= 600) {
          std::cout << "SIM PROJECTION: " << i
                    << " iterations, res=" << residual << "\n";
          break;
        }
        previous_residual = residual;
      }
    }

    for (int y = 1; y < height - 1; y++) {
      for (int x = 1; x < width - 1; x++) {
        vx.f(x, y) -= 0.5f * ih * (p.f(x + 1, y) - p.f(x - 1, y));
        vy.f(x, y) -= 0.5f * ih * (p.f(x, y + 1) - p.f(x, y - 1));
      }
    }
  }

  void setDT() {
    float max_vel_sq = 1.0e-7;
    for (int y = 1; y < height; y++) {
      for (int x = 1; x < width; x++) {
        max_vel_sq = std::max(
            max_vel_sq, vy.f(x, y) * vy.f(x, y) + vx.f(x, y) * vx.f(x, y));
      }
    }
    dt = pwidth / (width - 1.0f) / sqrt(max_vel_sq) * 1.3f;
    std::cout << "SIM SET_DT: max_vel=" << sqrt(max_vel_sq) << ", dt=" << dt
              << "\n";
  }

  void advect() {
    float h = pwidth / (width - 1);
    float ih = 1.0f / h;
    float max_sqdistance = 0.0f;
    for (int y = 1; y < height - 1; y++) {
      for (int x = 1; x < width - 1; x++) {
        float xf = x * h;
        float yf = y * h;
        float vtx = vx.f(x, y);
        float vty = vy.f(x, y);
        int steps = 3;
        for (int i = 0; i < steps; i++) {
          xf = std::max(std::min(xf - vtx * dt / steps, (width - 1.0f) * h),
                        0.0f);
          yf = std::max(std::min(yf - vty * dt / steps, (height - 1.0f) * h),
                        0.0f);
          int x0 = static_cast<int>(xf * ih);
          int y0 = static_cast<int>(yf * ih);
          float s = xf * ih - x0;
          float t = yf * ih - y0;
          float sqdistance =
              ((vtx * dt * ih / steps) * (vtx * dt * ih / steps) +
               (vty * dt * ih / steps) * (vty * dt * ih / steps));
          max_sqdistance = std::max(sqdistance, max_sqdistance);
          vtx =
              (vx.f(x0, y0) * (1.0f - s) + vx.f(x0 + 1, y0) * s) * (1.0f - t) +
              (vx.f(x0, y0 + 1) * (1.0f - s) + vx.f(x0 + 1, y0 + 1) * s) * t;
          vty =
              (vy.f(x0, y0) * (1.0f - s) + vy.f(x0 + 1, y0) * s) * (1.0f - t) +
              (vy.f(x0, y0 + 1) * (1.0f - s) + vy.f(x0 + 1, y0 + 1) * s) * t;
        }
        vx.b(x, y) = vtx;
        vy.b(x, y) = vty;
      }
    }

    std::cout << "SIM ADVECT: max_displacement = " << sqrt(max_sqdistance)
              << "\n";

    vx.swap();
    vy.swap();
  }

  void step() {
    setDT();
    diffuse(vx);
    diffuse(vy);

    advect();

    project();
    for (int y = 0; y < height; y++) {
      vx.f(width - 1, y) = vx.f(width - 2, y);
      vx.b(width - 1, y) = vx.f(width - 2, y);
    }
    std::cout << "\n";
  }

  float pwidth;
  float mu;
  const int width, height;
  float dt;

 private:
  DoubleBuffered2DGrid vx, vy, p;
};
