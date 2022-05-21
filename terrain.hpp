#ifndef TERRAIN_H_
#define TERRAIN_H_

#include "db2dgrid.hpp"
#include "external/lodepng.h"
#include <glm/vec2.hpp>
#include <vector>

class Terrain {
public:
  Terrain(std::string filename, int scale);

  void init(const Single2DGrid &flag);
  std::vector<float> generateLine(const Single2DGrid &flag);
  void shiftMap();
  void drawCircle(glm::vec2 ic, int diam, float val);
  float subSample(int ix, int iy);

  Single2DGrid flagFullRes;
  Single2DGrid flagSimRes;

  int scale;

  const int sliceWidth = 500;

  std::vector<Single2DGrid> slices;

  int currentSlice, nextSlice;
  int sliceProgress;
  int currentOffset;
  int nextOffset;
};

#endif // TERRAIN_H_
