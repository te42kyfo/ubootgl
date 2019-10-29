#pragma once
#include "entt/entity/registry.hpp"
#include "floating_item.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace DrawFloatingItems {

void init();

void draw(entt::registry &registry, entt::component component, Texture texture,
          glm::mat4 PVM, float magnification);

template <typename T>
void draw(T *begin, T *end, glm::mat4 PVM, float enhancement);
} // namespace DrawFloatingItems
