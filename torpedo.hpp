#include "floating_item.hpp"
#include <glm/gtx/vector_angle.hpp>

bool processTorpedo(Torpedo &t, FloatingItem *targetBegin,
                    FloatingItem *targetEnd, FloatingItem *playerBegin,
                    FloatingItem *playerEnd, float simTime) {

  t.age += simTime;
  FloatingItem *bestTarget = nullptr;
  float bestScore = 7.0;
  float bestAngle = 0.0;

  for (auto &target = targetBegin; target != targetEnd; target++) {
    float distance = length(t.pos - target->pos);
    float angle = glm::dot({cos(t.rotation), sin(t.rotation)},
                           (target->pos - t.pos) / distance);
    float score = angle * angle / distance / distance;
    if (distance > 0.3)
      score = 0.0f;


    if (distance < (target->size.x + target->size.y) * 0.6f)
        return true;

    if (score > bestScore) {
      bestScore = score;
      bestTarget = target;
      bestAngle = glm::orientedAngle({cos(t.rotation), sin(t.rotation)},
                                     (target->pos - t.pos) / distance);
    }
  }

  for (auto &target = playerBegin; target != playerEnd; target++) {
    if (target->player == t.player)
      continue;
    float distance = length(t.pos - target->pos);
    float angle = glm::dot({cos(t.rotation), sin(t.rotation)},
                           (target->pos - t.pos) / distance);
    float score = angle * angle / distance / distance;
    if (distance > 0.06)
      score = 0.0f;

    if (distance < (target->size.x + target->size.y) * 0.6f)
      return true;

    if (score > bestScore) {
      bestScore = score;
      bestTarget = target;
      bestAngle = glm::orientedAngle({cos(t.rotation), sin(t.rotation)},
                                     (target->pos - t.pos) / distance);
    }
  }

  if (t.age < 0.03f) {
    t.force = glm::vec2(cos(t.rotation), sin(t.rotation)) * 2.0f;
  } else if (t.age < 0.8f) {
    t.angVel = glm::sign(bestAngle) * 10.0;
    if (bestTarget != nullptr)
      t.force = glm::vec2(cos(t.rotation), sin(t.rotation)) * 4.0f;
    else
      t.force = glm::vec2(cos(t.rotation), sin(t.rotation)) * 1.0f; // *
  } else {
    return true;
  }

  if (t.bumpCount > 0)
    return true;

  return false;
}
