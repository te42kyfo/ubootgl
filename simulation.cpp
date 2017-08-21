#include "simulation.hpp"
#include <glm/glm.hpp>
#include <iostream>
using namespace glm;

void Simulation::interpolateFields() {
#pragma omp parallel for
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      vec2 iv = bilinearVel(vec2(x, y));
      ivx(x, y) = iv.x;
      ivy(x, y) = iv.y;
    }
  }
  staleInterpolatedFields = false;
}

float* Simulation::getVX() {
  if (staleInterpolatedFields) interpolateFields();
  return ivx.data();
}
float* Simulation::getVY() {
  if (staleInterpolatedFields) interpolateFields();
  return ivy.data();
}

// Fluid cells like this: a | b |
float Simulation::singlePBC(BC bc, float a) {
  if (bc == BC::INFLOW) return a;
  if (bc == BC::OUTFLOW) return a;
  if (bc == BC::OUTFLOW_ZERO_PRESSURE) return -a;
  if (bc == BC::NOSLIP) return a;
  assert(false);
  return 0.0;
}

void Simulation::setPBC() {
  for (int y = 0; y < height; y++) {
    p(0, y) = singlePBC(bcWest, p(1, y));
    p(width - 1, y) = singlePBC(bcEast, p(width - 2, y));
  }
  for (int x = 0; x < width; x++) {
    p(x, 0) = singlePBC(bcSouth, p(x, 1));
    p(x, height - 1) = singlePBC(bcNorth, p(x, height - 2));
  }
}

//         |
//   a>   b>
//         |
float Simulation::VBCPar(BC bc, float a, float b) {
  if (bc == BC::INFLOW) return b;
  if (bc == BC::OUTFLOW) return a;
  if (bc == BC::OUTFLOW_ZERO_PRESSURE) return a;
  if (bc == BC::NOSLIP) return 0;
  assert(false);
  return 0.0;
}

// -------
//   b>
// -------
//   a>
float Simulation::VBCPer(BC bc, float a, float b) {
  if (bc == BC::INFLOW) return b;
  if (bc == BC::OUTFLOW) return a;
  if (bc == BC::OUTFLOW_ZERO_PRESSURE) return a;
  if (bc == BC::NOSLIP) return -a;
  assert(false);
  return 0.0;
}

void Simulation::setVBCs() {
  for (int y = 0; y < vx.height; y++) {
    vx.f(0, y) = vx.b(0, y) = VBCPar(bcWest, vx.f(1, y), vx.f(0, y));
    vx.f(vx.width - 1, y) = vx.b(vx.width - 1, y) =
        VBCPar(bcEast, vx.f(vx.width - 2, y), vx.f(vx.width - 1, y));
  }
  for (int x = 0; x < vx.width; x++) {
    vx.f(x, 0) = vx.b(x, 0) = VBCPer(bcNorth, vx.f(x, 1), vx.f(x, 0));
    vx.f(x, vx.height - 1) = vx.b(x, vx.height - 1) =
        VBCPer(bcSouth, vx.f(x, vx.height - 2), vx.f(x, vx.height - 1));
  }

  for (int y = 0; y < vy.height; y++) {
    vy.f(0, y) = vy.b(0, y) = VBCPer(bcWest, vy.f(1, y), vy.f(0, y));
    vy.f(vy.width - 1, y) = vy.b(vy.width - 1, y) =
        VBCPer(bcEast, vy.f(vy.width - 2, y), vy.f(vy.width - 1, y));
  }
  for (int x = 0; x < vy.width; x++) {
    vy.f(x, 0) = vy.b(x, 0) = VBCPar(bcNorth, vy.f(x, 1), vy.f(x, 0));
    vy.f(x, vy.height - 1) = vy.b(x, vy.height - 1) =
        VBCPar(bcSouth, vy.f(x, vy.height - 2), vy.f(x, vy.height - 1));
  }
}

void Simulation::diffuse() {
  float a = dt * mu * (width - 1.0f) / pwidth;

  // x-velocity diffusion
  // use this timestep as initial guess
  vx.copyFrontToBack();
  for (int i = 1; i < 3; i++) {
    for (int y = 1; y < vx.height - 1; y++) {
      for (int x = 1; x < vx.width - 1; x++) {
        float val = 0;
        val += vx.b(x + 1, y) * flag(x + 1, y) * flag(x + 2, y);
        val += vx.b(x - 1, y) * flag(x, y) * flag(x - 1, y);

        float fvn = flag(x, y + 1) * flag(x - 1, y + 1);
        val += vx.b(x, y + 1) * fvn + (1.0 - fvn) * -vx.b(x, y);
        float fvs = flag(x, y - 1) * flag(x - 1, y - 1);
        val += vx.b(x, y - 1) * fvs + (1.0 - fvs) * -vx.b(x, y);

        vx.b(x, y) = flag(x, y) * flag(x + 1, y) * (vx.f(x, y) + a * val) /
                     (1.0f + 4.0f * a);
      }
    }
    setVBCs();
  }
  vx.swap();

  // y-velocity diffusion
  vy.copyFrontToBack();
  for (int i = 1; i < 3; i++) {
    for (int y = 1; y < vy.height - 1; y++) {
      for (int x = 1; x < vy.width - 1; x++) {
        float val = 0;
        val += vy.b(x, y - 1) * flag(x, y) * flag(x - 1, y);
        val += vy.b(x, y + 1) * flag(x, y + 1) * flag(x, y + 2);

        float fve = flag(x + 1, y) * flag(x + 1, y + 1);
        val += vy.b(x + 1, y) * fve + (1.0 - fve) * -vy.b(x, y);
        float fvw = flag(x - 1, y) * flag(x - 1, y + 1);
        val += vy.b(x - 1, y) * fvw + (1.0 - fvw) * -vy.b(x, y);

        vy.b(x, y) = flag(x, y) * flag(x, y + 1) * (vy.f(x, y) + a * val) /
                     (1.0f + 4.0f * a);
      }
    }
    setVBCs();
  }
  vy.swap();
}

void Simulation::project() {
  float h = pwidth / (width - 1.0f);
  float ih = 1.0f / h;

#pragma omp parallel for
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      f(x, y) = -ih * (vx(x, y) - vx(x - 1, y) + vy(x, y) - vy(x, y - 1));
    }
  }

  float residual = calculateResidualField(p, f, flag, r, h);
  diag << "PROJECT: res=" << residual << "\n";

  mg.solve(p, f, flag, h, true);

  centerP();
  setPBC();
  residual = calculateResidualField(p, f, flag, r, h);

  diag << "PROJECT: res=" << residual << "\n";
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 2; x++) {
      vx(x, y) -= flag(x, y) * flag(x + 1, y) * ih * (p(x + 1, y) - p(x, y));
    }
  }
  for (int y = 1; y < height - 2; y++) {
    for (int x = 1; x < width - 1; x++) {
      vy(x, y) -= flag(x, y) * flag(x, y + 1) * ih * (p(x, y + 1) - p(x, y));
    }
  }
}

void Simulation::setDT() {
  float max_vel_sq = 1.0e-7;
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      max_vel_sq =
          std::max(max_vel_sq, vy(x, y) * vy(x, y) + vx(x, y) * vx(x, y));
    }
  }
  dt = pwidth / (width - 1.0f) / sqrt(max_vel_sq) * 0.5f;
  diag << "SET_DT: Vmax=" << sqrt(max_vel_sq) << ", dt=" << dt << "\n";
}

vec2 Simulation::bilinearVel(vec2 c) {
  c.x = min(max(c.x, 0.51f), vx.width - 0.51f);
  c.y = min(max(c.y, 0.51f), vy.height - 0.51f);

  vec2 result;

  vec2 cx = c - vec2(0.5, 0.0);
  ivec2 ic = cx;
  vec2 st = cx - (vec2)ic;
  vec2 cst = vec2(1.0, 1.0) - st;

  result.x =
      cst.y * (cst.x * vx(ic.x, ic.y) + st.x * vx(ic.x + 1, ic.y)) +
      st.y * (cst.x * vx(ic.x, ic.y + 1) + st.x * vx(ic.x + 1, ic.y + 1));

  vec2 cy = c - vec2(0.0, 0.5);
  ic = cy;
  st = cy - (vec2)ic;
  cst = vec2(1.0, 1.0) - st;

  result.y =
      cst.y * (cst.x * vy(ic.x, ic.y) + st.x * vy(ic.x + 1, ic.y)) +
      st.y * (cst.x * vy(ic.x, ic.y + 1) + st.x * vy(ic.x + 1, ic.y + 1));

  return result;
}

void Simulation::advect() {
  float h = pwidth / (width - 1);
  float ih = 1.0f / h;
  const int steps = 2;
#pragma omp parallel for
  for (int y = 1; y < vx.height - 1; y++) {
    for (int x = 1; x < vx.width - 1; x++) {
      vec2 pos = vec2(x + 0.5, y);
      vec2 vel = bilinearVel(pos);
      for (int step = 0; step < steps; step++) {
        vec2 tempPos = pos - 0.5f * dt * ih / steps * vel;
        vec2 tempVel = bilinearVel(tempPos);
        pos -= dt * ih / steps * tempVel;
        vel = bilinearVel(pos);
      }
      vx.b(x, y) = vel.x;
    }
  }
#pragma omp parallel for
  for (int y = 1; y < vy.height - 1; y++) {
    for (int x = 1; x < vy.width - 1; x++) {
      vec2 pos = vec2(x, y + 0.5);
      vec2 vel = bilinearVel(pos);
      for (int step = 0; step < steps; step++) {
        vec2 tempPos = pos - 0.5f * dt * ih / steps * vel;
        vec2 tempVel = bilinearVel(tempPos);
        pos -= dt * ih / steps * tempVel;
        vel = bilinearVel(pos);
      }

      vy.b(x, y) = vel.y;
    }
  }
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

  diffuse();

  advect();
  setVBCs();

  project();
  setVBCs();

  diag << "\n";
  staleInterpolatedFields = true;
}
