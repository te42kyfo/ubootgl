#pragma once
#include "texture.hpp"
#include <glm/glm.hpp>

class FloatingItem {
public:
  FloatingItem()
      : size(1, 1), mass(1), angMass(1), force(0, 0), vel(0, 0), pos(0, 0),
        rotation(0.0), angVel(0.0){};

  FloatingItem(glm::vec2 size, float mass, glm::vec2 vel, glm::vec2 pos,
               float rotation, float angVel, Texture *tex)
      : size(size), mass(mass),
        angMass((size.x * size.x + size.y * size.y) * mass / 12.0f),
        force(0.0f), vel(vel), pos(pos), rotation(rotation), angVel(angVel),
        tex(tex){};

  glm::vec2 size;
  float mass;
  float angMass;

  glm::vec2 force;
  glm::vec2 vel;
  glm::vec2 pos;

  float rotation;
  float angVel;

  Texture *tex;
};
