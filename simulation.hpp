#pragma once
#include "db2dgrid.hpp"
#include "external/lodepng.h"
#include "floating_item.hpp"
#include "pressure_solver.hpp"
#include <cmath>
#include <functional>
#include <glm/vec2.hpp>
#include <iomanip>
#include <iostream>
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
        vx(width - 1, height), // staggered
        vy(width, height - 1), // grids
        p(width, height), f(width, height), flag(width, height),
        r(width, height), ivx(width, height), ivy(width, height),
        mg(width, height), h(pwidth / (width - 1.0f)), disx(0.0f, 1.0f),
        disy(0.0f, (float)height / width) {}

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
    p = {width, height};
    f = {width, height};
    flag = {width, height};
    r = {width, height};
    ivx = {width, height};
    ivy = {width, height};

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
    bcEast = BC::OUTFLOW;

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
  float *getVX();
  float *getVY();
  float *getFlag() { return flag.data(); }
  float *getP() { return p.data(); }
  float *getR() { return r.data(); }

  void setGrids(glm::ivec2 c, float val) {

    if (c.x >= 0 && c.y >= 0 && c.x < width && c.y < height) {
      flag(c) = val;
      if (val == 0) {
        vx(c) = 0.0;
        vy(c) = 0.0;
        p(c) = 0.0;
      }
    }
  }

  glm::vec2 bilinearVel(glm::vec2 c);

  float diffusion_l2_residual();
  void diffuse();
  void project();
  void centerP();
  float getDT();
  void advect();

  void step(float timestep);

  template <typename T> void advectFloatingItems(T *begin, T *end);

  float pwidth;
  float mu;
  int width, height;
  float dt;

  BC bcWest, bcEast, bcNorth, bcSouth;

  std::stringstream diag;

  DoubleBuffered2DGrid vx, vy;
  Single2DGrid p, f, flag, r;
  Single2DGrid ivx, ivy;
  bool staleInterpolatedFields = true;
  MG mg;
  float h;

  std::default_random_engine gen;
  std::uniform_real_distribution<float> disx;
  std::uniform_real_distribution<float> disy;

  std::vector<glm::vec3> sinks;
};
