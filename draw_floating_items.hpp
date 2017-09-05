#pragma once
#include <glm/glm.hpp>
#include <vector>
#include "floating_item.hpp"

namespace DrawFloatingItems {
void init();
void draw(std::vector<FloatingItem>& items, glm::mat4 PVM);
}
