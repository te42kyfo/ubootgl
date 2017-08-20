#pragma once

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <vector>
#include <assert.h>

class Single2DGrid {
 public:
  Single2DGrid(int width, int height)
    : width(width), height(height), v(width * height, 0.0f) {}

  Single2DGrid() : width(0), height(0){};

  int idx(int x, int y) { return y * width + x; }
  float* data() { return v.data(); }

  float& operator()(int x, int y) {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[idx(x, y)];
  }
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
  float& f(int x, int y) {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[front][idx(x, y)];
  }
  float& b(int x, int y) {
    assert(x < width && x >= 0 && y < height && y >= 0);
    return v[back][idx(x, y)];
  }
  float* data() { return v[front].data(); }
  void copyFrontToBack() {
    for (int i = 0; i < width * height; i++) {
      v[back][i] = v[front][i];
    }
  }
  float& operator()(int x, int y) { return f(x, y); }

 private:
  int width, height;
  int front, back;
  std::vector<float> v[2];
};
