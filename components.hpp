#pragma once
#include "entt/entity/registry.hpp"
#include "texture.hpp"
#include <glm/glm.hpp>

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
  entt::entity player;
};

struct CoTorpedo {
  float age = 0.0f;
};

struct CoExplosion {
  CoExplosion(float explosionDiam, int fragmentLevel)
      : age(0.0f), explosionDiam(explosionDiam), fragmentLevel(fragmentLevel){};
  float age;
  float explosionDiam;
  bool fragmented = false;
  int fragmentLevel = 0;
};

enum class PLAYER_STATE { ALIVE, ALIVE_PROTECTED, RESPAWNING, KILLED };

struct CoPlayer {
  CoPlayer(int keySet) : keySet(keySet){};
  float torpedoCooldown = 0.0;
  float torpedosLoaded = 10.0;
  float timer = 0.1f;
  PLAYER_STATE state = PLAYER_STATE::ALIVE_PROTECTED;
  int deaths = 0;
  int kills = 0;
  int torpedosFired = 0;
  int keySet = 0;
};

struct CoAgent {};

struct CoRespawnsOoB {};
struct CoDeletedOoB {};
struct CoDecays {
  float halflife;
};

struct CoTarget {};

struct CoHasTracer {};

struct CoAnimated {
  float frame;
};

inline float &frame(entt::registry &registry, entt::entity entity) {
  return registry.get<CoAnimated>(entity).frame;
}
