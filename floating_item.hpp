#pragma once
#include <glm/glm.hpp>
#include "texture.hpp"

class FloatingItem {
 public:
  FloatingItem()
      : force(0, 0), vel(0, 0), pos(0, 0), mass(1), size(1, 1), rotation(0.0){};
  FloatingItem(glm::vec2 force, glm::vec2 vel, glm::vec2 pos, float mass,
               glm::vec2 size, float rotation, Texture* tex)
      : force(force),
        vel(vel),
        pos(pos),
        mass(mass),
        size(size),
        rotation(rotation),
        tex(tex){};

  glm::vec2 force;
  glm::vec2 vel;
  glm::vec2 pos;

  float mass;
  glm::vec2 size;
  float rotation;

  Texture* tex;
};
