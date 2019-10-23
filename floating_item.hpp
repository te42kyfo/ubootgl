#pragma once
#include "texture.hpp"
#include <glm/glm.hpp>

class FloatingItem {
public:
  FloatingItem()
      : size(1, 1), mass(1), angMass(1), force(0, 0), vel(0, 0), pos(0, 0),
        rotation(0.0), angVel(0.0), angForce(0), bumpCount(0), frame(0.0f),
        age(0.0){};

  FloatingItem(glm::vec2 size, float mass, glm::vec2 vel, glm::vec2 pos,
               float rotation, float angVel, Texture *tex, int player = -1)
      : size(size), mass(mass),
        angMass((size.x * size.x + size.y * size.y) * mass / 12.0f),
        force(0.0f), vel(vel), pos(pos), rotation(rotation), angVel(angVel),
        angForce(0), bumpCount(0), tex(tex), frame(0.0f), age(0.0),
        player(player){};

  glm::vec2 size;
  float mass;
  float angMass;

  glm::vec2 force;
  glm::vec2 vel;
  glm::vec2 pos;

  float rotation;
  float angVel;
  float angForce;
  int bumpCount;

  Texture *tex;
  float frame;
  double age;

  int player;
};

class Torpedo : public FloatingItem {
public:
  Torpedo(glm::vec2 size, float mass, glm::vec2 vel, glm::vec2 pos,
          float rotation, float angVel, Texture *tex, int player)
      : FloatingItem(size, mass, vel, pos, rotation, angVel, tex, player){};
};

struct CoItem {
  glm::vec2 size;
  glm::vec2 pos;
  float rotation;
};

struct CoKinematics {
  CoKinematics(float mass, glm::vec2 vel, float angVel)
      : mass(mass), vel(vel), force({0.0f, 0.0f}), angVel(angVel),
        angForce(0.0f), bumpCount(0){};

  float mass;

  glm::vec2 vel;
  glm::vec2 force;

  float angVel;
  float angForce;

  int bumpCount;
};

struct CoSprite {
  Texture *tex;
  float frame;
};

struct CoPlayerAligned {
  int player;
};

struct CoTorpedo {};

struct CoAgent {};

struct CoExplosion {};

struct CoPlayer {
  CoPlayer(int keySet) : keySet(keySet){};
  float torpedoCooldown = 0.0;
  float torpedosLoaded = 10.0;
  float deathtimer = 0.0f;
  int deaths = 0;
  int kills = 0;
  int torpedosFired = 0;
  int keySet = 0;
};

struct CoRespawnsOoB {};
struct CoDeletedOoB {};
struct CoDecays {
  int halflife;
};
