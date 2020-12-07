#include "simulation.hpp"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <iomanip>
#include <iostream>
#include <vector>

using glm::vec2;

void Simulation::interpolateFields() {
  // #pragma omp parallel for
  for (int y = 0; y < ivx.height; y++) {
    for (int x = 1; x < ivx.width - 1; x++) {
      ivx(x, y) = 0.5f * (vx.f(x, y) + vx.f(x + 1, y));
    }
  }

  for (int y = 1; y < ivx.height - 1; y++) {
    for (int x = 0; x < ivx.width; x++) {
      ivy(x, y) = 0.5f * (vx.f(x, y + 1) + vx.f(x, y));
    }
  }
}

float *Simulation::getVX() { return ivx.data(); }
float *Simulation::getVY() { return ivy.data(); }

// Fluid cells like this: a | b |
float Simulation::singlePBC(BC bc, float a) {
  if (bc == BC::INFLOW)
    return a;
  if (bc == BC::OUTFLOW)
    return a;
  if (bc == BC::OUTFLOW_ZERO_PRESSURE)
    return -a;
  if (bc == BC::NOSLIP)
    return a;
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
  if (bc == BC::INFLOW)
    return b;
  if (bc == BC::OUTFLOW)
    return a;
  if (bc == BC::OUTFLOW_ZERO_PRESSURE)
    return a;
  if (bc == BC::NOSLIP)
    return 0;
  assert(false);
  return 0.0;
}

// -------
//   b>
// -------
//   a>
float Simulation::VBCPer(BC bc, float a, float b) {
  if (bc == BC::INFLOW)
    return b;
  if (bc == BC::OUTFLOW)
    return a;
  if (bc == BC::OUTFLOW_ZERO_PRESSURE)
    return a;
  if (bc == BC::NOSLIP)
    return -a;
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
    vx.f(x, 0) = vx.b(x, 0) = VBCPer(bcSouth, vx.f(x, 1), vx.f(x, 0));
    vx.f(x, vx.height - 1) = vx.b(x, vx.height - 1) =
        VBCPer(bcNorth, vx.f(x, vx.height - 2), vx.f(x, vx.height - 1));
  }

  for (int y = 0; y < vy.height; y++) {
    vy.f(0, y) = vy.b(0, y) = VBCPer(bcWest, vy.f(1, y), vy.f(0, y));
    vy.f(vy.width - 1, y) = vy.b(vy.width - 1, y) =
        VBCPer(bcEast, vy.f(vy.width - 2, y), vy.f(vy.width - 1, y));
  }
  for (int x = 0; x < vy.width; x++) {
    vy.f(x, 0) = vy.b(x, 0) = VBCPar(bcSouth, vy.f(x, 1), vy.f(x, 0));
    vy.f(x, vy.height - 1) = vy.b(x, vy.height - 1) =
        VBCPar(bcNorth, vy.f(x, vy.height - 2), vy.f(x, vy.height - 1));
  }
}

void Simulation::diffuse() {
  float a = dt * mu * (width - 1.0f) / pwidth;

#pragma omp parallel
  {
    // x-velocity diffusion
    // use this timestep as initial guess
    vx.copyFrontToBack();
    for (int i = 1; i < 3; i++) {
#pragma omp for
      for (int y = 1; y < vx.height - 1; y++) {
        for (int x = 1; x < vx.width - 1; x++) {
          float val = 0;
          val += vx.f(x + 1, y) * flag(x + 1, y) * flag(x + 2, y);
          val += vx.f(x - 1, y) * flag(x, y) * flag(x - 1, y);

          float fvn = flag(x, y + 1) * flag(x - 1, y + 1);
          val += vx.f(x, y + 1) * fvn + (1.0f - fvn) * -vx.f(x, y);
          float fvs = flag(x, y - 1) * flag(x - 1, y - 1);
          val += vx.f(x, y - 1) * fvs + (1.0f - fvs) * -vx.f(x, y);

          vx.b(x, y) = flag(x, y) * flag(x + 1, y) * (vx.f(x, y) + a * val) /
                       (1.0f + 4.0f * a);
        }
      }
#pragma omp single
      vx.swap();
      setVBCs();
#pragma omp barrier
    }

    // y-velocity diffusion
    vy.copyFrontToBack();
    for (int i = 1; i < 3; i++) {
#pragma omp for
      for (int y = 1; y < vy.height - 1; y++) {
        for (int x = 1; x < vy.width - 1; x++) {
          float val = 0;
          val += vy.f(x, y - 1) * flag(x, y) * flag(x, y - 1);
          val += vy.f(x, y + 1) * flag(x, y + 1) * flag(x, y + 2);

          float fve = flag(x + 1, y) * flag(x + 1, y + 1);
          val += vy.f(x + 1, y) * fve + (1.0 - fve) * -vy.f(x, y);
          float fvw = flag(x - 1, y) * flag(x - 1, y + 1);
          val += vy.f(x - 1, y) * fvw + (1.0 - fvw) * -vy.f(x, y);

          vy.b(x, y) = flag(x, y) * flag(x, y + 1) * (vy.f(x, y) + a * val) /
                       (1.0f + 4.0f * a);
        }
      }
#pragma omp single
      vy.swap();
      setVBCs();
#pragma omp barrier
    }
  }
}

void Simulation::project() {
  float ih = 1.0f / h;
#pragma omp parallel for
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      f(x, y) = -ih * (vx(x, y) - vx(x - 1, y) + vy(x, y) - vy(x, y - 1));
    }
  }

  for (auto &sink : sinks) {
    auto gC = glm::vec2(sink) / h + 0.5f;
    if (gC.x <= 3 || gC.x > width - 3 || gC.y <= 3 || gC.y > height - 3)
      continue;
    for (int y = -1; y <= 1; y++) {
      for (int x = -1; x <= 1; x++) {
        f(gC.x + x, gC.y + y) = sink.z;
      }
    }
    sink.z *= 0.05f;
  }

  sinks.erase(
      remove_if(begin(sinks), end(sinks), [=](auto &t) { return t.z < 0.05f; }),
      end(sinks));

  // float residual = calculateResidualField(p, f, flag, r, h);
  // diag << "PROJECT: res=" << residual << "\n";

  mg.solve(p, f, flag, h, true);
  // rbgs(p, f, flag, h, 1.6);

  centerP();

  setPBC();

  // residual = calculateResidualField(p, f, flag, r, h);
  // diag << "PROJECT: res=" << residual << "\n";
#pragma omp parallel for
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 2; x++) {
      vx(x, y) -= flag(x, y) * flag(x + 1, y) * ih * (p(x + 1, y) - p(x, y));
    }
  }
#pragma omp parallel for
  for (int y = 1; y < height - 2; y++) {
    for (int x = 1; x < width - 1; x++) {
      vy(x, y) -= flag(x, y) * flag(x, y + 1) * ih * (p(x, y + 1) - p(x, y));
    }
  }
}

void Simulation::centerP() {
  float avgP = 0;
#pragma omp parallel for reduction(+ : avgP)
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      avgP += p(x, y);
    }
  }
#pragma omp parallel for
  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      p(x, y) -= avgP / width / height;
    }
  }
}

float Simulation::getDT() {
  float max_vel = 1.0e-7;

  for (int y = 1; y < height - 1; y++) {
    for (int x = 1; x < width - 1; x++) {
      max_vel = glm::max(max_vel, length(bilinearVel({x, y})));
    }
  }

  float rDT = pwidth / (width - 1.0f) / max_vel * 2.5f;
  diag << "SET_DT: Vmax=" << max_vel << ", dt=" << rDT << "\n";
  return rDT;
}

template <typename T> vec2 inline bilinearGrid(vec2 c, T &vx, T &vy) {

  vec2 result;
  vec2 cx = c - vec2(0.5, 0.0);

  cx.x = glm::min(glm::max(cx.x, 0.0f), vx.width - 1.01f);
  cx.y = glm::min(glm::max(cx.y, 0.0f), vx.height - 1.01f);

  glm::ivec2 ic = cx;

  vec2 st = fract(cx);

  result.x = glm::mix(
      glm::mix(vx(ic.x, ic.y), vx(ic.x + 1, ic.y), st.x),
      glm::mix(vx(ic.x, ic.y + 1), vx(ic.x + 1, ic.y + 1), st.x), st.y);

  vec2 cy = c - vec2(0.0, 0.5);
  cy.x = glm::min(glm::max(cy.x, 0.0f), vy.width - 1.01f);
  cy.y = glm::min(glm::max(cy.y, 0.0f), vy.height - 1.01f);

  ic = cy;

  st = fract(cy);

  result.y = glm::mix(
      glm::mix(vy(ic.x, ic.y), vy(ic.x + 1, ic.y), st.x),
      glm::mix(vy(ic.x, ic.y + 1), vy(ic.x + 1, ic.y + 1), st.x), st.y);

  return result;
}

vec2 inline Simulation::bilinearVel(vec2 c) { return bilinearGrid(c, vx, vy); }

void Simulation::advect() {
  float ih = 1.0f / h;

#pragma omp parallel
  {
#pragma omp for
    for (int y = 1; y < vx.height - 1; y++) {
      for (int x = 1; x < vx.width - 1; x++) {
        vec2 pos = vec2(x + 0.5, y);

        vec2 vel = vec2(vx(x, y), 0.25f * (vy(x, y) + vy(x, y - 1) +
                                           vy(x + 1, y) + vy(x + 1, y - 1)));

        pos -= dt * ih * bilinearVel(pos - 0.5f * dt * ih * vel);
        vel = bilinearVel(pos);

        // pos.x = glm::min(glm::max(0.0f, pos.x), (float)vx.width);
        // pos.y = glm::min(glm::max(0.0f, pos.y), (float)vx.height);

        vx.b(x, y) = flag(x, y) * flag(x + 1, y) * vel.x;
        //(0.5f * vx((int)(pos.x), (int)(pos.y + 0.5f)) + 0.5f * vel.x);
      }
    }
#pragma omp for
    for (int y = 1; y < vy.height - 1; y++) {
      for (int x = 1; x < vy.width - 1; x++) {
        vec2 pos = vec2(x, y + 0.5);
        vec2 vel = vec2(
            0.25f * (vx(x, y) + vx(x, y + 1) + vx(x - 1, y) + vx(x - 1, y + 1)),
            vy(x, y));

        pos -= dt * ih * bilinearVel(pos - 0.5f * dt * ih * vel);
        vel = bilinearVel(pos);

        pos.x = glm::min(glm::max(0.0f, pos.x), (float)vy.width);
        pos.y = glm::min(glm::max(0.0f, pos.y), (float)vy.height);

        vy.b(x, y) =
            flag(x, y) * flag(x, y + 1) *
            (0.5f * vy((int)(pos.x + 0.5f), (int)(pos.y)) + 0.5f * vel.y);
      }
    }

#pragma omp single
    {
      vx.swap();
      vy.swap();
    }
  }
}

void Simulation::step(float timestep) {
  dt = timestep;
  diag.str("SIM\n");

  diag << "Time Step: dt=" << timestep << "\n";
  // getDT();

  applyAccumulatedVelocity();

  diffuse();

  advect();
  setVBCs();

  project();
  setVBCs();

  diag << "\n";
  interpolateFields();
}

glm::vec2 Simulation::psampleFlagNormal(glm::vec2 pc) {
  float p01 = psampleFlagLinear(glm::vec2(pc.x - h, pc.y + h));
  float p11 = psampleFlagLinear(glm::vec2(pc.x + h, pc.y + h));
  float p00 = psampleFlagLinear(glm::vec2(pc.x - h, pc.y - h));
  float p10 = psampleFlagLinear(glm::vec2(pc.x + h, pc.y - h));

  return glm::vec2(p11 + p10 - p01 - p00, p01 + p11 - p00 - p10);
}

float Simulation::psampleFlagLinear(glm::vec2 pc) {

  glm::vec2 c = pc / (pwidth / (flag.width)) - 0.5f;
  glm::ivec2 ic = c;
  ic = glm::max({0, 0}, glm::min({flag.width - 1, flag.height - 1}, ic));
  glm::vec2 st = fract(c);

  float p01 = flag(ic + glm::ivec2{0, 1});
  float p11 = flag(ic + glm::ivec2{1, 1});
  float p00 = flag(ic + glm::ivec2{0, 0});
  float p10 = flag(ic + glm::ivec2{1, 0});

  return glm::mix(glm::mix(p00, p10, st.x), glm::mix(p01, p11, st.x), st.y);
}

float Simulation::psampleFlagNearest(glm::vec2 pc) {
  glm::vec2 gridPos = pc / (pwidth / (flag.width));
  return flag(gridPos);
}

void Simulation::applyAccumulatedVelocity() {
  std::scoped_lock lock(accum_mutex);
#pragma omp parallel
  {
#pragma omp for
    for (int y = 1; y < vx.height - 1; y++) {
      for (int x = 1; x < vx.width - 1; x++) {
        vx(x, y) += vx_accum(x, y);
        vx_accum(x, y) = 0;
      }
    }

#pragma omp for
    for (int y = 1; y < vy.height - 1; y++) {
      for (int x = 1; x < vy.width - 1; x++) {
        vy(x, y) += vy_accum(x, y);
        vy_accum(x, y) = 0;
      }
    }
  }
}

void Simulation::advectFloatingItems(entt::registry &registry, float gameDT) {
  auto floatingItemView = registry.view<CoItem, CoKinematics>();
  std::scoped_lock lock(accum_mutex);
  for (auto entity : floatingItemView) {
    auto &item = floatingItemView.get<CoItem>(entity);
    auto &kin = floatingItemView.get<CoKinematics>(entity);

    int iterationSteps =
        fmin(5.0f, fmax(1.0f, glm::max(abs(kin.vel.x), abs(kin.vel.y)) *
                                  gameDT / h * 2.5));

    float subDT = gameDT / iterationSteps;

    for (int step = 0; step < iterationSteps; step++) {
      glm::vec2 posBefore = item.pos;

      // Position update
      item.pos += subDT * kin.vel;

      item.rotation =
          fmod(item.rotation + subDT * kin.angVel + 2 * M_PI, 2 * M_PI);

      glm::vec2 gridPos = item.pos / (pwidth / (flag.width));

      // Skip out of Bounds objects
      if (gridPos.x >= width - 2 || gridPos.x <= 1.0 ||
          gridPos.y >= height - 2 || gridPos.y <= 1.0)
        // return;
        continue;

      // Check terrain collision
      if (psampleFlagLinear(item.pos) < 0.5f) {
        glm::vec2 surfaceNormal =
            normalize(psampleFlagNormal(0.5f * (posBefore + item.pos)));
        if (psampleFlagLinear(posBefore) > 0.5f)
          item.pos = posBefore;
        if (length(surfaceNormal) > 0.0f) {
          if (dot(kin.vel, surfaceNormal) < 0.0)
            kin.vel = reflect(kin.vel, surfaceNormal) * 0.7f;
          kin.vel += surfaceNormal * 0.001f;
          if (dot(kin.force, surfaceNormal) < 0.0f)
            kin.force += dot(kin.force, surfaceNormal) * surfaceNormal;
        } else {
          kin.vel = vec2(0.0f);
          // kin.force = vec2(0.0f);
        }
        kin.bumpCount++;
      }

      glm::vec2 externalForce = glm::vec2(0.0, -0.5) * kin.mass + kin.force;
      glm::vec2 centralForce = glm::vec2(0.0, 0.0);
      float angForce = kin.angForce;

      glm::vec2 surfacePoints[] = {
          {-1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, -1.0f}, {0.0f, 1.0f}};

      float sideLength[] = {item.size.y, item.size.y, item.size.x, item.size.x};

      for (int i = 0; i < 4; i++) {
        auto sp = surfacePoints[i];

        auto tSP = sp * item.size;
        tSP = rotate(tSP, item.rotation);
        tSP = tSP + item.pos;
        auto sampleVel = bilinearVel(tSP / h);
        auto deltaVel = sampleVel - kin.vel;
        auto force = rotate(sp, item.rotation) *
                     glm::min(0.0f, dot(deltaVel, rotate(sp, item.rotation)));

        centralForce +=
            force * 2000.0f * (300.0f * sideLength[i] + 1.0f) * sideLength[i];
      }
      centralForce += (bilinearVel(item.pos / h) - kin.vel) * 6.0f;

      // Calculate new velocity
      kin.vel += subDT * (externalForce + centralForce) / kin.mass;

      float angMass = item.size.x * item.size.y * kin.mass * (1.0f / 12.0f);
      kin.angVel += subDT * angForce / angMass;

      gridPos = item.pos / (pwidth / (flag.width));
      if (gridPos.x > 0 && gridPos.x < width - 1 && gridPos.y > 0 &&
          gridPos.y < height - 1 && flag(gridPos) > 0.0f) {
        float area = item.size.x * item.size.y / (h * h);
        auto deltaVel = bilinearVel(item.pos / h) +
                        bilinearGrid(item.pos / h, vx_accum, vy_accum) -
                        kin.vel;

        auto deltaVec = deltaVel * area * subDT * 20.0f;

        glm::vec2 cx = item.pos / h - vec2(0.5f, 0.0);
        glm::ivec2 ic = cx;
        vec2 st = fract(cx);
        vx_accum(ic + glm::ivec2{1, 1}) -= st.x * st.y * deltaVec.x *
                                           flag(ic + glm::ivec2{1, 1}) *
                                           flag(ic + glm::ivec2{2, 1});
        vx_accum(ic + glm::ivec2{0, 1}) -= (1.0f - st.x) * st.y * deltaVec.x *
                                           flag(ic + glm::ivec2{0, 1}) *
                                           flag(ic + glm::ivec2{1, 1});
        vx_accum(ic + glm::ivec2{1, 0}) -= st.x * (1.0f - st.y) * deltaVec.x *
                                           flag(ic + glm::ivec2{1, 0}) *
                                           flag(ic + glm::ivec2{2, 0});
        vx_accum(ic + glm::ivec2{0, 0}) -=
            (1.0f - st.x) * (1.0f - st.y) * deltaVec.x *
            flag(ic + glm::ivec2{0, 0}) * flag(ic + glm::ivec2{1, 0});
        assert(!std::isnan(vx_accum(ic)));

        glm::vec2 cy = item.pos / h - vec2(0.0f, 0.5);
        ic = cy;
        st = fract(cy);
        vy_accum(ic + glm::ivec2{1, 1}) -= st.x * st.y * deltaVec.y *
                                           flag(ic + glm::ivec2{1, 1}) *
                                           flag(ic + glm::ivec2{1, 2});
        vy_accum(ic + glm::ivec2{0, 1}) -= (1.0f - st.x) * st.y * deltaVec.y *
                                           flag(ic + glm::ivec2{0, 1}) *
                                           flag(ic + glm::ivec2{0, 2});
        vy_accum(ic + glm::ivec2{1, 0}) -= st.x * (1.0f - st.y) * deltaVec.y *
                                           flag(ic + glm::ivec2{1, 0}) *
                                           flag(ic + glm::ivec2{1, 1});
        vy_accum(ic + glm::ivec2{0, 0}) -=
            (1.0f - st.x) * (1.0f - st.y) * deltaVec.y *
            flag(ic + glm::ivec2{0, 1}) * flag(ic + glm::ivec2{0, 2});
        assert(!std::isnan(vy_accum(ic)));
      }
    }

    kin.angForce = 0;
    kin.force = {0, 0};
  }
}
