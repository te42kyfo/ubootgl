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
float bilinearSample(const GridType &grid, glm::vec2 c) {
  glm::ivec2 ic = c;
  glm::vec2 st = fract(c);

  float v = glm::mix(
      glm::mix(grid(ic.x, ic.y), grid(ic.x + 1, ic.y), st.x),
      glm::mix(grid(ic.x, ic.y + 1), grid(ic.x + 1, ic.y + 1), st.x), st.y);
  return v;
}

class DoubleBuffered2DGrid {
public:
  DoubleBuffered2DGrid(int width, int height)
      : width(width), height(height), front(0),
        back(1), v{std::vector<float>(width * height),
                   std::vector<float>(width * height)} {}

  DoubleBuffered2DGrid() : width(0), height(0){};

  int idx(int x, int y) { return y * width + x; }
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

  float *data() { return v[front].data(); }
  void copyFrontToBack() {
#pragma omp for
    for (int i = 0; i < width * height; i++) {
      v[back][i] = v[front][i];
    }
  }
  float &operator()(int x, int y) { return f(x, y); }
  float &operator()(glm::ivec2 c) { return f(c.x, c.y); }

  int width, height;

private:
  int front, back;
  std::vector<float> v[2];
};
