#include "db2dgrid.hpp"

void gs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, int nx, int ny,
        float h, float alpha);
void rbgs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, int nx, int ny,
          float h, float alpha);
float calculateResidualField(Single2DGrid& p, Single2DGrid& f,
                             Single2DGrid& flag, Single2DGrid& r, int nx,
                             int ny, float h);
