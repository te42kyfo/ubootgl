#include "db2dgrid.hpp"

void gs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
        float alpha);
void rbgs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
          float alpha);
void mg(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
        bool zeroGradientBC = false);

float calculateResidualField(Single2DGrid& p, Single2DGrid& f,
                             Single2DGrid& flag, Single2DGrid& r, float h);

class MG {
 public:
  MG() {};
  MG(int width, int height) : width(width), height(height) {
    int currentWidth = width;
    int currentHeight = height;

    while (currentWidth > 3 && currentHeight > 3) {
      rs.push_back(Single2DGrid(currentWidth, currentHeight));
      rcs.push_back(Single2DGrid(currentWidth, currentHeight));
      ecs.push_back(Single2DGrid(currentWidth, currentHeight));
      es.push_back(Single2DGrid(currentWidth, currentHeight));
      flagcs.push_back(Single2DGrid(currentWidth, currentHeight));
      flagcs.back() = 1.0;
      currentWidth /= 2;
      currentHeight /= 2;
    }
    levels = rs.size();
  }

  void solve(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
             bool zeroGradientBC = false) {
    solveLevel(p, f, flag, h, 0, zeroGradientBC);
  };

  void solveLevel(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
                  int level, bool zeroGradientBC = false);

  std::vector<Single2DGrid> rs;
  std::vector<Single2DGrid> rcs;
  std::vector<Single2DGrid> ecs;
  std::vector<Single2DGrid> es;
  std::vector<Single2DGrid> flagcs;
  int levels;
  int width;
  int height;
};
