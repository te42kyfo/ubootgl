#pragma once

#include <vector>

class Single2DGrid {
 public:
  Single2DGrid(int width, int height)
      : width(width), height(height), v(width * height) {}

  Single2DGrid() : width(0), height(0){};

  int idx(int x, int y) { return y * width + x; }
  float* data() { return v.data(); }

  float& operator()(int x, int y) { return v[idx(x, y)]; }

 private:
  int width, height;
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
  float& f(int x, int y) { return v[front][idx(x, y)]; }
  float& b(int x, int y) { return v[back][idx(x, y)]; }
  float* data() { return v[front].data(); }
  void copyFrontToBack() {
    for (int i = 0; i < width * height; i++) {
      v[back][i] = v[front][i];
    }
  }
  float& operator()(int x, int y) { return v[front][idx(x, y)]; }

  
 private:
  int width, height;
  int front, back;
  std::vector<float> v[2];
};
