#pragma once
#include <vector>

class Simulation {
 public:
  Simulation(int width, int height)
      : width(width),
        height(height),
        vx{std::vector<float>(width * height),
           std::vector<float>(width * height)},
        vy{std::vector<float>(width * height),
           std::vector<float>(width * height)},
        p{std::vector<float>(width * height),
          std::vector<float>(width * height)},
        front(0),
        back(1) {
    for (int x = 0; x < width; x++) {
      vx[back][x] = vx[front][x] = 2.0;
    }
    for (int x = 0; x < width; x++) {
      vx[back][width*height-x-1] = vx[front][width*height-x-1] = -2.0;
    }
  }

  float* getVX() { return vx[front].data(); }
  float* getVY() { return vy[front].data(); }
  float* getP() { return p[front].data(); }

  void step() {
    for (int y = 1; y < height - 1; y++) {
      for (int x = 1; x < width - 1; x++) {
        vx[back][y * width + x] =
            (vx[front][(y + 1) * width + x] + vx[front][y * width + x + 1] +
             vx[front][(y - 1) * width + x] + vx[front][y * width + x - 1]) *
            (1.0f / 4.0f);
      }
    }
    std::swap(front, back);
  }

  int width, height;

 private:
  std::vector<float> vx[2], vy[2], p[2];
  int front;
  int back;
};
