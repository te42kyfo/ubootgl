#pragma once
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "lodepng.h"

class DoubleBuffered2DGrid {
 public:
  DoubleBuffered2DGrid(int width, int height)
      : width(width),
        height(height),
        front(0),
        back(1),
        v{std::vector<float>(width * height),
          std::vector<float>(width * height)} {}

  DoubleBuffered2DGrid() : width(0), height(0){};

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
  int width, height;
  int front, back;
  std::vector<float> v[2];
};

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
        flag(width, height) {}

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
    flag = {width, height};

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        int idx = y * width + x;
        if (rgba_image[idx] == rgba{0, 0, 0, 255}) {
          flag.f(x, y) = 0.0f;
        } else {
          flag.f(x, y) = 1.0f;
        }
      }
    }
    bcWest = BC::INFLOW;
    bcEast = BC::OUTFLOW;
    bcNorth = BC::NOSLIP;
    bcSouth = BC::NOSLIP;
    for (int y = 0; y < height; y++) {
      vx.f(0, y) = 10;
      vx.b(0, y) = 10;
      vx.f(width - 1, y) = 0;
      vx.b(width - 1, y) = 0;
    }
  }

  enum class BC { INFLOW, OUTFLOW, OUTFLOW_ZERO_PRESSURE, NOSLIP };

  void setPBC();
  void setVBC();

  float* getVX() { return vx.data(); }
  float* getVY() { return vy.data(); }
  float* getFlag() { return flag.data(); }
  float* getP() { return p.data(); }

  float diffusion_l2_residual(DoubleBuffered2DGrid& v);
  void diffuse(DoubleBuffered2DGrid& v);
  float projection_residual();
  void project();
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
  DoubleBuffered2DGrid vx, vy, p, flag;
};
