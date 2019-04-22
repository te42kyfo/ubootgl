#include "floating_item.hpp"
#include <glm/gtx/vector_angle.hpp>

bool processTorpedo(Torpedo &t, FloatingItem *targetBegin,
                    FloatingItem *targetEnd, float simTime) {

  t.age += simTime;
  FloatingItem *bestTarget = nullptr;
  float bestScore = 10.0;
  float bestAngle = 0.0;
  for (auto &target = targetBegin; target != targetEnd; target++) {
    float distance = length(t.pos - target->pos);
    float angle = glm::dot({cos(t.rotation), sin(t.rotation)},
                           (target->pos - t.pos) / distance);
    float score = angle * angle / distance;
    if (distance > 0.08)
      continue;

    if (distance < 0.002)
      return true;

    if (score > bestScore) {
      bestScore = score;
      bestTarget = target;
      bestAngle = glm::orientedAngle({cos(t.rotation), sin(t.rotation)},
                                     (target->pos - t.pos) / distance);
    }
  }

  if (t.age < 0.05f) {
    t.force = glm::vec2(cos(t.rotation), sin(t.rotation)) * 4.0f;
  } else if (t.age < 0.3f) {
    t.angForce = glm::sign(bestAngle) * 0.00004;
    if (bestTarget != nullptr)
      t.force = glm::vec2(cos(t.rotation), sin(t.rotation)) * 8.0f; // *
  } else {
    return true;
  }
  return false;
}
