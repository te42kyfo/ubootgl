#pragma once
#include <cmath>
#include <functional>
#include <glm/vec2.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "db2dgrid.hpp"
#include "lodepng.h"
#include "pressure_solver.hpp"

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
        vx(width - 1, height),  // staggered
        vy(width, height - 1),  // grids
        p(width, height),
        f(width, height),
        flag(width, height),
        r(width, height),
        ivx(width, height),
        ivy(width, height),
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

    vx = {width - 1, height};  // staggered
    vy = {width, height - 1};  // grids
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
    bcWest = BC::INFLOW;
    bcEast = BC::OUTFLOW;
    bcNorth = BC::NOSLIP;
    bcSouth = BC::NOSLIP;
    for (int y = 0; y < vx.height; y++) {
      vx.f(0, y) = vx.b(0, y) = 1.0;
      vx.f(vx.width - 1, y) = vx.b(vx.width - 1, y) = 0;
    }


    mg = MG(flag);
  }

  enum class BC { INFLOW, OUTFLOW, OUTFLOW_ZERO_PRESSURE, NOSLIP };

  float singlePBC(BC bc, float b);
  void setPBC();
  float VBCPar(BC bc, float a, float b);
  float VBCPer(BC bc, float a, float b);
  void setVBCs();

  void interpolateFields();
  float* getVX();
  float* getVY();
  float* getFlag() { return flag.data(); }
  float* getP() { return p.data(); }
  float* getR() { return r.data(); }

  glm::vec2 bilinearVel(glm::vec2 c);

  float diffusion_l2_residual();
  void diffuse();
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
  Single2DGrid ivx, ivy;
  bool staleInterpolatedFields = true;
  MG mg;
};
