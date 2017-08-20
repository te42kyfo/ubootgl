#pragma once
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "pressure_solver.hpp"
#include "db2dgrid.hpp"
#include "lodepng.h"

struct rgba {
  unsigned char r, g, b, a;
  bool operator==(const rgba& o) {
    return r == o.r && g == o.g && b == o.b && a == o.a;
  }
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
        p(width, height),
        f(width, height),
        flag(width, height),
        r(width, height),
        mg(width, height) {}

  Simulation(std::string filename, float pwidth, float mu)
      : pwidth(pwidth), mu(mu) {
    std::vector<unsigned char> image;
    unsigned image_width, image_height;

    unsigned error =
        lodepng::decode(image, image_width, image_height, filename);
    rgba* rgba_image = reinterpret_cast<rgba*>(image.data());

    if (error)
      std::cout << "decoder error " << error << ": "
                << lodepng_error_text(error) << std::endl;

    width = image_width;
    height = image_height;

    vx = {width, height};
    vy = {width, height};
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
    bcWest = BC::INFLOW;
    bcEast = BC::OUTFLOW;
    bcNorth = BC::NOSLIP;
    bcSouth = BC::NOSLIP;
    for (int y = 0; y < height; y++) {
      vx.f(0, y) = 1;
      vx.b(0, y) = 1;
      vx.f(width - 1, y) = 0;
      vx.b(width - 1, y) = 0;
      flag(0, y) = 0.0;
      flag(width - 1, y) = 0.0;
    }
    for (int x = 0; x < width; x++) {
      flag(x, 0) = 1.0;
      flag(x, height - 1) = 1.0;
    }
    mg = MG(flag);
  }

  enum class BC { INFLOW, OUTFLOW, OUTFLOW_ZERO_PRESSURE, NOSLIP };

  float setSinglePBC(BC bc, float a, float b);
  void setPBC();
  void setVBC();

  float* getVX() { return vx.data(); }
  float* getVY() { return vy.data(); }
  float* getFlag() { return flag.data(); }
  float* getP() { return p.data(); }
  float* getR() { return r.data(); }

  float diffusion_l2_residual(DoubleBuffered2DGrid& v);
  void diffuse(DoubleBuffered2DGrid& v);
  void project();
  void centerP();
  void setDT();
  void advect();

  void step();

  float pwidth;
  float mu;
  int width, height;
  float dt;

  BC bcWest, bcEast, bcNorth, bcSouth;

  std::stringstream diag;

 private:
  DoubleBuffered2DGrid vx, vy;
  Single2DGrid p, f, flag, r;
  MG mg;
};
