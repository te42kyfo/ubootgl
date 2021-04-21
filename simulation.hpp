#pragma once
#include "components.hpp"
#include "db2dgrid.hpp"
#include "entt/entity/registry.hpp"
#include "external/lodepng.h"
#include "pressure_solver.hpp"
#include <cmath>
#include <functional>
#include <glm/vec2.hpp>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <vector>

struct rgba {
  unsigned char r, g, b, a;
  bool operator==(const rgba &o) {
    return r == o.r && g == o.g && b == o.b && a == o.a;
  }
};

class Simulation {
public:
  Simulation(float pwidth, float mu, int width, int height)
      : pwidth(pwidth), mu(mu), width(width), height(height),
        vx(width - 1, height),         // staggered
        vy(width, height - 1),         // grids
        vx_accum(width - 1, height),   // grids
        vy_accum(width, height - 1),   // grids
        vx_current(width - 1, height), // grids
        vy_current(width, height - 1), // grids
        p(width, height), f(width, height), flag(width, height),
        r(width, height), mg(width, height), h(pwidth / (width - 1.0f)),
        disx(0.0f, 1.0f), disy(0.0f, (float)height / width) {}

  Simulation(std::string filename, float pwidth, float mu)
      : pwidth(pwidth), mu(mu) {
    std::vector<unsigned char> image;
    unsigned image_width, image_height;

    unsigned error =
        lodepng::decode(image, image_width, image_height, filename);
    rgba *rgba_image = reinterpret_cast<rgba *>(image.data());

    if (error)
      std::cout << "decoder error " << error << ": "
                << lodepng_error_text(error) << std::endl;

    width = image_width;
    height = image_height;

    vx = {width - 1, height}; // staggered
    vy = {width, height - 1}; // grids
    vx_accum = {width - 1, height};
    vy_accum = {width, height - 1};
    vx_current = {width - 1, height};
    vy_current = {width, height - 1};
    p = {width, height};
    f = {width, height};
    flag = {width, height};
    r = {width, height};

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        int idx = (height - y - 1) * width + x;
        if (rgba_image[idx] == rgba{0, 0, 0, 255}) {
          flag(x, y) = 0.0f;
        } else {
          flag(x, y) = 1.0f;
        }
      }
    }
    bcSouth = BC::NOSLIP;
    bcNorth = BC::NOSLIP;
    bcWest = BC::INFLOW;
    bcEast = BC::OUTFLOW_ZERO_PRESSURE;

    for (int x = 0; x < vy.width; x++) {
      vy.f(x, 0) = vy.b(x, 0) = 0.0;
    }
    for (int y = 0; y < vx.height; y++) {
      vx.f(0, y) = vx.b(0, y) = 1.0;
    }

    mg = MG(flag);
    h = (pwidth / (width - 1.0f));

    disx = std::uniform_real_distribution<float>(0.0f, 1.0f);
    disy = std::uniform_real_distribution<float>(0.0f, (float)height / width);
  }

  enum class BC { INFLOW, OUTFLOW, OUTFLOW_ZERO_PRESSURE, NOSLIP };

  float singlePBC(BC bc, float b);
  void setPBC();
  float VBCPar(BC bc, float a, float b);
  float VBCPer(BC bc, float a, float b);
  void setVBCs();

  void interpolateFields();
  float *getFlag() { return flag.data(); }
  float *getP() { return p.data(); }
  float *getR() { return r.data(); }

  void setGrids(glm::ivec2 c, float val) {

    if (c.x >= 0 && c.y >= 0 && c.x < width && c.y < height) {
      flag(c) = val;
      if (val == 0) {
        vx(c) = 0.0;
        vx(c + glm::ivec2(-1, 0)) = 0.0;
        vy(c) = 0.0;
        vy(c + glm::ivec2(0, -1)) = 0.0;
        p(c) = 0.0;
      }
    }
  }

  glm::vec2 bilinearVel(glm::vec2 c);
  float psampleFlagNearest(glm::vec2 pc);
  float psampleFlagLinear(glm::vec2 pc);
  glm::vec2 psampleFlagNormal(glm::vec2 pc);

  float diffusion_l2_residual();
  void diffuse();
  void project();
  void centerP();
  float getDT();
  void advect();
  void applyAccumulatedVelocity();
  void saveCurrentVelocityFields();

  void step(float timestep);

  void advectFloatingItems(entt::registry &registry, float gameDT);

  float pwidth;
  float mu;
  int width, height;
  float dt;

  BC bcWest, bcEast, bcNorth, bcSouth;

  std::stringstream diag;

  DoubleBuffered2DGrid vx, vy;
  Single2DGrid vx_accum, vy_accum;
  Single2DGrid vx_current, vy_current;
  std::mutex accum_mutex;
  Single2DGrid p, f, flag, r;
  MG mg;
  float h;

  std::default_random_engine gen;
  std::uniform_real_distribution<float> disx;
  std::uniform_real_distribution<float> disy;

  std::vector<glm::vec3> sinks;
};
