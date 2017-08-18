#include "db2dgrid.hpp"

void gs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
        float alpha);
void rbgs(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
          float alpha);
void mg(Single2DGrid& p, Single2DGrid& f, Single2DGrid& flag, float h,
        bool zeroGradientBC = false);

float calculateResidualField(Single2DGrid& p, Single2DGrid& f,
                             Single2DGrid& flag, Single2DGrid& r, float h);
