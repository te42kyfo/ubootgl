#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "floating_item.hpp"

namespace DrawFloatingItems {
void init();
  void draw( FloatingItem* begin, FloatingItem* end, glm::mat4 PVM);
}
