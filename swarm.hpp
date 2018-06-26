#pragma once
#include "db2dgrid.hpp"
#include "floating_item.hpp"

class Swarm {
public:
  void addAgent(FloatingItem agent);
  void update(FloatingItem ship, const Single2DGrid &flag, float h);
  std::vector<FloatingItem> agents;

};
