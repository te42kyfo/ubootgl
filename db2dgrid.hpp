#pragma once

#include <algorithm>
#include <assert.h>
#include <glm/glm.hpp>
#include <iomanip>
#include <iostream>
#include <vector>

class Single2DGrid {
public:
  Single2DGrid(int width, int height)
      : width(width), height(height), v(width * height, 0.0f) {}

  Single2DGrid() : width(0), height(0){};

  int idx(int x, int y) const { return y * width + x; }
  float *data() { return v.data(); }

  float &operator()(int x, int y) {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[idx(x, y)];
  }
  float operator()(int x, int y) const {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[idx(x, y)];
  }

  float &operator()(glm::ivec2 c) { return (*this)(c.x, c.y); }
  float operator()(glm::ivec2 c) const { return (*this)(c.x, c.y); }

  float operator=(float val) {
    std::fill(std::begin(v), std::end(v), val);
    return val;
  }
  int width, height;

  void print() {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        std::cout << std::setprecision(2) << std::setw(2) << (*this)(x, y)
                  << " ";
      }
      std::cout << "\n";
    }
  }

private:
  std::vector<float> v;
};

template <typename GridType>
inline float bilinearSample(const GridType &grid, glm::vec2 c) {

  c = glm::clamp(c, glm::vec2(1.0f, 1.0f),
                 glm::vec2(grid.width - 1.0f, grid.height - 1.0f));

  glm::ivec2 ic = c;
  glm::vec2 st = fract(c);

  float v1 = grid(ic.x, ic.y);
  float v2 = grid(ic.x + 1, ic.y);
  float v3 = grid(ic.x, ic.y + 1);
  float v4 = grid(ic.x + 1, ic.y + 1);

  float vm1 = v1 + (v2 - v1) * st.x;
  float vm2 = v3 + (v4 - v3) * st.x;
  return vm1 + (vm2 - vm1) * st.y;
}

class DoubleBuffered2DGrid {
public:
  DoubleBuffered2DGrid(int width, int height)
      : width(width), height(height), front(0),
        back(1), v{std::vector<float>(width * height),
                   std::vector<float>(width * height)} {}

  DoubleBuffered2DGrid() : width(0), height(0){};

  int idx(int x, int y) const { return y * width + x; }
  void swap() { std::swap(front, back); }

  float &f(int x, int y) {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[front][idx(x, y)];
  }
  float &b(int x, int y) {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[back][idx(x, y)];
  }
  float &f(glm::ivec2 c) { return f(c.x, c.y); }
  float &b(glm::ivec2 c) { return b(c.x, c.y); }

  float f(int x, int y) const {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[front][idx(x, y)];
  }
  float b(int x, int y) const {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[back][idx(x, y)];
  }
  float f(glm::ivec2 c) const { return f(c.x, c.y); }
  float b(glm::ivec2 c) const { return b(c.x, c.y); }

  float &operator()(int x, int y) { return f(x, y); }
  float &operator()(glm::ivec2 c) { return f(c.x, c.y); }

  float operator()(int x, int y) const { return f(x, y); }
  float operator()(glm::ivec2 c) const { return f(c.x, c.y); }

  void copyFrontToBack() {
#pragma omp for
    for (int i = 0; i < width * height; i++) {
      v[back][i] = v[front][i];
    }
  }

  float *data() { return v[front].data(); }
  int width, height;

private:
  int front, back;
  std::vector<float> v[2];
};
