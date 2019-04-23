#pragma once
#include "floating_item.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace DrawFloatingItems {

void init();
template <typename T>
void draw(T *begin, T *end, glm::mat4 PVM);
} // namespace DrawFloatingItems
