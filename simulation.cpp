#include "simulation.hpp"
#include "interpolators.hpp"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/string_cast.hpp>
#include <immintrin.h>
#include <iomanip>
#include <iostream>
#include <vector>

using glm::vec2;


void Simulation::saveCurrentVelocityFields() {
  memcpy(vx_current.data(), vx.data(), vx.width * vx.height * sizeof(float));
  memcpy(vy_current.data(), vy.data(), vy.width * vy.height * sizeof(float));
}


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
    return fmax(a, 0.0f);
  if (bc == BC::OUTFLOW_ZERO_PRESSURE)
    return fmax(a, 0.0f);
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
    return fmax(a, 0.0f);
  if (bc == BC::OUTFLOW_ZERO_PRESSURE)
    return fmax(a, 0.0f);
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
    // vx.copyFrontToBack();
    for (int i = 1; i < 3; i++) {
#pragma omp for
      for (int y = 1; y < vx.height - 1; y++) {
#pragma omp simd
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
    // vy.copyFrontToBack();
    for (int i = 1; i < 3; i++) {
#pragma omp for
      for (int y = 1; y < vy.height - 1; y++) {
#pragma omp simd
        for (int x = 1; x < vy.width - 1; x++) {
          float val = 0;
          val += vy.f(x, y - 1) * flag(x, y) * flag(x, y - 1);
          val += vy.f(x, y + 1) * flag(x, y + 1) * flag(x, y + 2);

          float fve = flag(x + 1, y) * flag(x + 1, y + 1);
          val += vy.f(x + 1, y) * fve + (1.0f - fve) * -vy.f(x, y);
          float fvw = flag(x - 1, y) * flag(x - 1, y + 1);
          val += vy.f(x - 1, y) * fvw + (1.0f - fvw) * -vy.f(x, y);

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
    sink.z *= pow(0.000001, dt * 50);
  }

  sinks.erase(
      remove_if(begin(sinks), end(sinks), [=](auto &t) { return t.z < 0.05f; }),
      end(sinks));

  mg.solve(p, f, flag, h, true);
  mg.solve(p, f, flag, h, true);

  //centerP();

  setPBC();

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
      max_vel = glm::max(max_vel, glm::max(vx(x,y), vy(x,y)));
    }
  }

  float rDT = pwidth / (width - 1.0f) / max_vel * 2.5f;
  diag << "SET_DT: Vmax=" << max_vel << ", dt=" << rDT << "\n";
  return rDT;
}


void Simulation::advect() {
  float ih = 1.0f / h;

#pragma omp parallel
  {
#pragma omp for schedule(dynamic, 16)
    for (int y = 1; y < vx.height - 1; y++) {
      for (int x = 1; x < vx.width - 8; x += 8) {
        __m256i mmx = _mm256_add_epi32(
            _mm256_set1_epi32(x), _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7));
        __m256i mmy = _mm256_set1_epi32(y);


        if(_mm256_movemask_ps( _mm256_cmp_ps(_mm256_add_ps(_mm256_loadu_ps(flag.data() + x - 1 + y * flag.width),
                                                           _mm256_loadu_ps(flag.data() + x + y * flag.width)),
                                             _mm256_set1_ps(2.0f), 0)) == 0) continue;



        __m256 posx =
            _mm256_add_ps(_mm256_cvtepi32_ps(mmx), _mm256_set1_ps(0.5f));
        __m256 posy = _mm256_cvtepi32_ps(mmy);

        __m256 vx1 = _mm256_loadu_ps(vx.data() + x + y * vx.width);
        __m256 vy1 = _mm256_mul_ps(
            _mm256_add_ps(
                _mm256_add_ps(
                    _mm256_loadu_ps(vy.data() + x + y * vy.width),
                    _mm256_loadu_ps(vy.data() + x + (y - 1) * vy.width)),
                _mm256_add_ps(
                    _mm256_loadu_ps(vy.data() + x + 1 + y * vy.width),
                    _mm256_loadu_ps(vy.data() + x + 1 + (y - 1) * vy.width))),
            _mm256_set1_ps(0.25f));

        __m256 midPosx = _mm256_sub_ps(
            posx, _mm256_mul_ps(vx1, _mm256_set1_ps(0.5f * dt * ih)));
        __m256 midPosy = _mm256_sub_ps(
            posy, _mm256_mul_ps(vy1, _mm256_set1_ps(0.5f * dt * ih)));

        __m256 vx2 = bicubicSample(
            vx, _mm256_sub_ps(midPosx, _mm256_set1_ps(0.5f)), midPosy);
        __m256 vy2 = bicubicSample(
            vy, midPosx, _mm256_sub_ps(midPosy, _mm256_set1_ps(0.5f)));

        __m256 endPosx =
            _mm256_sub_ps(posx, _mm256_mul_ps(vx2, _mm256_set1_ps(dt * ih)));
        __m256 endPosy =
            _mm256_sub_ps(posy, _mm256_mul_ps(vy2, _mm256_set1_ps(dt * ih)));

        __m256 xvel = bicubicSample(vx, endPosx - 0.5f, endPosy);

        xvel = _mm256_mul_ps(
            _mm256_mul_ps(xvel,
                          _mm256_loadu_ps(flag.data() + y * flag.width + x)),
            _mm256_loadu_ps(flag.data() + y * flag.width + x + 1));

        _mm256_storeu_ps(vx.back_data() + vx.width * y + x, xvel);
      }

      for (int x = 1; x < vy.width - 8; x += 8) {

        if(_mm256_movemask_ps( _mm256_cmp_ps(_mm256_add_ps(_mm256_loadu_ps(flag.data() + x  + y * flag.width),
                                                           _mm256_loadu_ps(flag.data() + x + (y-1) * flag.width)),
                                             _mm256_set1_ps(2.0f), 0)) == 0) continue;

        __m256i mmx = _mm256_add_epi32(
            _mm256_set1_epi32(x), _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7));
        __m256i mmy = _mm256_set1_epi32(y);

        __m256 posy =
            _mm256_add_ps(_mm256_cvtepi32_ps(mmy), _mm256_set1_ps(0.5f));
        __m256 posx = _mm256_cvtepi32_ps(mmx);

        __m256 vy1 = _mm256_loadu_ps(vy.data() + x + y * vy.width);
        __m256 vx1 = _mm256_mul_ps(
            _mm256_add_ps(
                _mm256_add_ps(
                    _mm256_loadu_ps(vx.data() + x + y * vx.width),
                    _mm256_loadu_ps(vx.data() + x + (y - 1) * vx.width)),
                _mm256_add_ps(
                    _mm256_loadu_ps(vx.data() + x + 1 + y * vx.width),
                    _mm256_loadu_ps(vx.data() + x + 1 + (y - 1) * vx.width))),
            _mm256_set1_ps(0.25f));

        __m256 midPosx = _mm256_sub_ps(
            posx, _mm256_mul_ps(vx1, _mm256_set1_ps(0.5f * dt * ih)));
        __m256 midPosy = _mm256_sub_ps(
            posy, _mm256_mul_ps(vy1, _mm256_set1_ps(0.5f * dt * ih)));

        __m256 vx2 = bicubicSample(
            vx, _mm256_sub_ps(midPosx, _mm256_set1_ps(0.5f)), midPosy);
        __m256 vy2 = bicubicSample(
            vy, midPosx, _mm256_sub_ps(midPosy, _mm256_set1_ps(0.5f)));

        __m256 endPosx =
            _mm256_sub_ps(posx, _mm256_mul_ps(vx2, _mm256_set1_ps(dt * ih)));
        __m256 endPosy =
            _mm256_sub_ps(posy, _mm256_mul_ps(vy2, _mm256_set1_ps(dt * ih)));

        __m256 yvel = bicubicSample(vy, endPosx, endPosy - 0.5f);

        yvel = _mm256_mul_ps(
            _mm256_mul_ps(yvel,
                          _mm256_loadu_ps(flag.data() + y * flag.width + x)),
            _mm256_loadu_ps(flag.data() + (y + 1) * flag.width + x));

        _mm256_storeu_ps(vy.back_data() + vy.width * y + x, yvel);
      }
    }

  }
  vx.swap();
  vy.swap();
}

void Simulation::step(float timestep) {
  dt = timestep;
  diag.str("SIM\n");

  diag << "Time Step: dt=" << timestep << "\n";

  applyAccumulatedVelocity();

  diffuse();

  advect();
  setVBCs();

  project();
  setVBCs();

  diag << "\n";
  saveCurrentVelocityFields();
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

float Simulation::psampleFlagLinear(glm::vec2 pc) {

  glm::vec2 c = pc / (pwidth / (flag.width)) - 0.5f;
  glm::ivec2 ic = c;
  ic = glm::max({0, 0}, glm::min({flag.width - 2, flag.height - 2}, ic));
  glm::vec2 st = fract(c);

  float p01 = flag(ic + glm::ivec2{0, 1});
  float p11 = flag(ic + glm::ivec2{1, 1});
  float p00 = flag(ic + glm::ivec2{0, 0});
  float p10 = flag(ic + glm::ivec2{1, 0});

  return glm::mix(glm::mix(p00, p10, st.x), glm::mix(p01, p11, st.x), st.y);
}

glm::vec2 Simulation::psampleFlagNormal(glm::vec2 pc) {
  float p01 = psampleFlagLinear(glm::vec2(pc.x - h, pc.y + h));
  float p11 = psampleFlagLinear(glm::vec2(pc.x + h, pc.y + h));
  float p00 = psampleFlagLinear(glm::vec2(pc.x - h, pc.y - h));
  float p10 = psampleFlagLinear(glm::vec2(pc.x + h, pc.y - h));

  return glm::vec2(p11 + p10 - p01 - p00, p01 + p11 - p00 - p10);
}

float Simulation::psampleFlagNearest(glm::vec2 pc) {
  glm::vec2 gridPos = pc / (pwidth / (flag.width));
  return flag(gridPos);
}

