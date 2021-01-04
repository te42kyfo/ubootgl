#include "floating_item.hpp"
#include <glm/gtx/vector_angle.hpp>

void UbootGlApp::processTorpedos() {

  float explosionDiam = 0.007;
  registry.view<CoTorpedo, CoItem, CoKinematics, CoPlayerAligned>().each(
      [&](auto entity, auto &torpedo, auto &item, auto &kin,
          auto &playerAligned) {
        torpedo.age += gameTimeStep;
        entt::entity bestTarget = entt::null;
        float bestScore = 5.0;
        float bestAngle = 0.0;

        bool explodes = false;

        registry.view<CoTarget, CoItem>().less([&](auto target,
                                                   auto &target_item) {
          if (playerAligned.player == target)
            return;
          if (registry.has<CoPlayerAligned>(target) &&
              registry.get<CoPlayerAligned>(target).player ==
                  playerAligned.player)
            return;

          float distance = length(item.pos - target_item.pos);
          float angle = glm::dot({cos(item.rotation), sin(item.rotation)},
                                 (target_item.pos - item.pos) / distance);
          float score = angle / distance / distance *
                        (target_item.size.x + target_item.size.y) * 20000.0f;

          float explosionRadiusModifier =
              registry.has<CoPlayer>(target) ? 0.5f : 1.5f;

          if (distance <
              (explosionDiam * 0.5 + (size(target).x + size(target).y) * 0.5) *
                  explosionRadiusModifier)
            explodes = true;

          if (score > bestScore) {
            bestScore = score;
            bestTarget = target;
            bestAngle =
                glm::orientedAngle({cos(item.rotation), sin(item.rotation)},
                                   (target_item.pos - item.pos) / distance);
          }
        });

        if (torpedo.age < 0.03f) {
          kin.force = glm::vec2(cos(item.rotation), sin(item.rotation)) * 4.0f;
        } else if (torpedo.age < 0.4f) {
          kin.angVel = glm::sign(bestAngle) * 7.0;
          if (bestTarget != entt::null)
            kin.force =
                glm::vec2(cos(item.rotation), sin(item.rotation)) * 7.0f;
          else
            kin.force =
                glm::vec2(cos(item.rotation), sin(item.rotation)) * 4.0f;
        } else {
          explodes = true;
        }
        if (kin.bumpCount > 0)
          explodes = true;

        if (explodes) {
          newExplosion(item.pos, explosionDiam, playerAligned.player);
          registry.destroy(entity);
        }
      });
}
