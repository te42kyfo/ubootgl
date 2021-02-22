#ifndef __TERRAIN_GENERATOR_H_
#define __TERRAIN_GENERATOR_H_

#include <vector>
#include "db2dgrid.hpp"

namespace TerrainGenerator {

void learn(const Single2DGrid& flag);

std::vector<float> generateLine( const Single2DGrid& flag);

}

#endif // __TERRAIN_GENERATOR_H_
