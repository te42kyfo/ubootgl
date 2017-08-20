#include "simulation.hpp"
#include <iostream>
#include "dtime.hpp"

// Fluid cells like this: a b | c
float Simulation::setSinglePBC(BC bc, float a, float b) {
  if (bc == BC::INFLOW) return b;
  if (bc == BC::OUTFLOW) return b;
  if (bc == BC::OUTFLOW_ZERO_PRESSURE) return 0.0f;
  if (bc == BC::NOSLIP) return b;
}

void Simulation::setPBC() {
  for (int y = 0; y < height; y++) {
    p(0, y) = setSinglePBC(bcWest, p(2, y), p(1, y));
    p(width - 1, y) = setSinglePBC(bcEast, p(width - 3, y), p(width - 2, y));
  }
  for (int x = 0; x < width; x++) {
    p(x, 0) = setSinglePBC(bcSouth, p(x, 2), p(x, 1));
    p(x, height - 1) =
        setSinglePBC(bcNorth, p(x, height - 3), p(x, height - 2));
  }
}

void Simulation::setVBC() {
  for (int y = 0; y < height; y++) {
    if (bcWest == BC::OUTFLOW || bcWest == BC::OUTFLOW_ZERO_PRESSURE)
      vx.f(0, y) = vx.b(0, y) = vx.f(1, y);
    if (bcEast == BC::OUTFLOW || bcEast == BC::OUTFLOW_ZERO_PRESSURE)
      vx.f(width - 1, y) = vx.b(width - 1, y) = vx.f(width - 2, y);
  }
  for (int x = 0; x < width; x++) {
    if (bcNorth == BC::OUTFLOW || bcNorth == BC::OUTFLOW_ZERO_PRESSURE)
      vy.f(x, 0) = vy.b(x, 0) = vy.f(x, 1);
    if (bcSouth == BC::OUTFLOW || bcSouth == BC::OUTFLOW_ZERO_PRESSURE)
      vy.f(x, height - 1) = vy.b(x, height - 1) = vy.f(x, height - 2);
  }
}

float Simulation::diffusion_l2_residual(DoubleBuffered2DGrid& v) {
  float a = dt * mu * pwidth / (width - 1.0f);
  float residual = 0.0;
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      float lres = (v.f(x, y) +
                    a * (v.b(x + 1, y) + v.b(x - 1, y) + v.b(x, y + 1) +
                         v.b(x, y - 1))) /
                       (1.0f + 4.0f * a) -
                   v.b(x, y);
      residual += flag(x, y) * lres * lres;
    }
  }
  return sqrt(residual) / width / height;
}

void Simulation::diffuse(DoubleBuffered2DGrid& v) {
  // use this timestep as initial guess
  v.copyFrontToBack();
  float a = dt * mu * (width - 1.0f) / pwidth;
  float omega = 1.0f;
  for (int i = 1; true; i++) {
    for (int y = 1; y < height - 1; y++) {
      for (int x = 1; x < width - 1; x++) {
        v.b(x, y) =
            flag(x, y) * (v.b(x, y) * (1.0f - omega) +
                          omega * (v.f(x, y) +
                                   a * (v.b(x + 1, y) + v.b(x - 1, y) +
                                        v.b(x, y + 1) + v.b(x, y - 1))) /
                              (1.0f + 4.0f * a));
      }
    }
    if (i > 2) {
      float res = diffusion_l2_residual(v);
      if (true || res < 1.0e-9 || i >= 100) {
        diag << std::setprecision(1) << std::scientific << "DIFF:     " << i
             << " iters, res=" << res << "\n";
        break;
      }
    }
  }
  v.swap();
}

void Simulation::project() {
  float h = pwidth / (width - 1.0f);
  float ih = 1.0f / h;

#pragma omp parallel for
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      f(x, y) = -0.5f * ih * (vx.f(x + 1, y) - vx.f(x - 1, y) + vy.f(x, y + 1) -
                              vy.f(x, y - 1));
    }
  }


  float residual = calculateResidualField(p, f, flag, r, h);
  diag << "PROJECT: res=" << residual << "\n";

  // for (int i = 0; i < 10; i++) {
  //   rbgs(p, f, flag, h, 1.0);
  //   setPBC();
  // }

  mg.solve(p, f, flag, h, true);

  centerP();
  setPBC();
  residual = calculateResidualField(p, f, flag, r, h);

  diag << "PROJECT: res=" << residual << "\n";
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      vx.f(x, y) -= flag(x, y) * 0.5f * ih * (p(x + 1, y) - p(x - 1, y));
      vy.f(x, y) -= flag(x, y) * 0.5f * ih * (p(x, y + 1) - p(x, y - 1));
    }
  }
}

void Simulation::setDT() {
  float max_vel_sq = 1.0e-7;
  for (int y = 1; y < height; y++) {
    for (int x = 1; x < width; x++) {
      max_vel_sq = std::max(max_vel_sq,
                            vy.f(x, y) * vy.f(x, y) + vx.f(x, y) * vx.f(x, y));
    }
  }
  dt = pwidth / (width - 1.0f) / sqrt(max_vel_sq) * 1.0f;
  diag << "SET_DT: Vmax=" << sqrt(max_vel_sq) << ", dt=" << dt << "\n";
}

void Simulation::advect() {
  float h = pwidth / (width - 1);
  float ih = 1.0f / h;
  float max_sqdistance = 0.0f;
#pragma omp parallel for
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      float xf = x * h;
      float yf = y * h;
      float vtx = vx.f(x, y);
      float vty = vy.f(x, y);
      int steps = 3;
      for (int i = 0; i < steps; i++) {
        xf =
            std::max(std::min(xf - vtx * dt / steps, (width - 1.0f) * h), 0.0f);
        yf = std::max(std::min(yf - vty * dt / steps, (height - 1.0f) * h),
                      0.0f);
        int x0 = static_cast<int>(xf * ih);
        int y0 = static_cast<int>(yf * ih);
        float s = xf * ih - x0;
        float t = yf * ih - y0;
        float sqdistance = ((vtx * dt * ih / steps) * (vtx * dt * ih / steps) +
                            (vty * dt * ih / steps) * (vty * dt * ih / steps));
        max_sqdistance = std::max(sqdistance, max_sqdistance);
        vtx = (vx.f(x0, y0) * (1.0f - s) + vx.f(x0 + 1, y0) * s) * (1.0f - t) +
              (vx.f(x0, y0 + 1) * (1.0f - s) + vx.f(x0 + 1, y0 + 1) * s) * t;
        vty = (vy.f(x0, y0) * (1.0f - s) + vy.f(x0 + 1, y0) * s) * (1.0f - t) +
              (vy.f(x0, y0 + 1) * (1.0f - s) + vy.f(x0 + 1, y0 + 1) * s) * t;
      }
      vx.b(x, y) = vtx * flag(x, y);
      vy.b(x, y) = vty * flag(x, y);
    }
  }

  diag << "ADVECT: Dmax=" << sqrt(max_sqdistance) << "\n";

  vx.swap();
  vy.swap();
}

void Simulation::centerP() {
  float avgP = 0;
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      avgP += p(x, y);
    }
  }
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      p(x, y) -= avgP / width / height;
    }
  }
}

void Simulation::step() {
  diag.str("SIM\n");
  setDT();

  diffuse(vx);
  diffuse(vy);
  setVBC();

  advect();
  setVBC();

  project();
  setVBC();

  diag << "\n";
}
