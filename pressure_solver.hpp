#include "db2dgrid.hpp"

void gs(Single2DGrid &p, Single2DGrid &f, Single2DGrid &flag, float h,
        float alpha);
void rbgs(Single2DGrid &p, Single2DGrid &f, Single2DGrid &flag, float h,
          float alpha);
void mg(Single2DGrid &p, Single2DGrid &f, Single2DGrid &flag, float h,
        bool zeroGradientBC = false);

float calculateResidualField(Single2DGrid &p, Single2DGrid &f,
                             Single2DGrid &flag, Single2DGrid &r, float h);

class MG {
public:
  MG(){};
  MG(int width, int height) : width(width), height(height) {
    int currentWidth = width;
    int currentHeight = height;

    while (currentWidth > 3 && currentHeight > 3) {
      rs.push_back(Single2DGrid(currentWidth, currentHeight));
      rcs.push_back(Single2DGrid(currentWidth, currentHeight));
      ecs.push_back(Single2DGrid(currentWidth, currentHeight));
      es.push_back(Single2DGrid(currentWidth, currentHeight));
      flagcs.push_back(Single2DGrid(currentWidth, currentHeight));
      flagcs.back().fill(1.0);
      currentWidth /= 2;
      currentHeight /= 2;
    }
    levels = rs.size();
  }
  MG(Single2DGrid &flag) : MG(flag.width, flag.height) { updateFields(flag); }

  void updateFields(Single2DGrid &flag) {
    flagcs[0] = flag;
    for (int level = 1; level < levels; level++) {
      auto &fc = flagcs[level];
      auto &f = flagcs[level - 1];
      fc.fill(1.0);
      for (int y = 1; y < fc.height - 1; y++) {
        for (int x = 1; x < fc.width - 1; x++) {
          float v = 0.0;
          v = f(2 * x - 1, 2 * y - 1) * 1 + f(2 * x + 0, 2 * y - 1) * 2 +
              f(2 * x + 1, 2 * y - 1) * 1;
          v += f(2 * x - 1, 2 * y + 0) * 2 + f(2 * x + 0, 2 * y + 0) * 4 +
               f(2 * x + 1, 2 * y + 0) * 2;
          v += f(2 * x - 1, 2 * y + 1) * 1 + f(2 * x + 0, 2 * y + 1) * 2 +
               f(2 * x + 1, 2 * y + 1) * 1;
          v *= 1.0f / 16.0f;
          if (v > 0.2)
            fc(x, y) = 1.0;
          else
            fc(x, y) = 0.0;
        }
      }
    }
  }

  void solve(Single2DGrid &p, Single2DGrid &f, Single2DGrid &flag, float h,
             bool zeroGradientBC = false) {
    solveLevel(p, f, flag, h, 0, zeroGradientBC);
  };

private:
  void solveLevel(Single2DGrid &p, Single2DGrid &f, Single2DGrid &flag, float h,
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
