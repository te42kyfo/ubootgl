#include "components.hpp"
#include "ubootgl_app.hpp"
#include <glm/gtx/vector_angle.hpp>

void UbootGlApp::launchTorpedo(entt::entity pEnt) {

  auto playerItem = registry.get<CoItem>(pEnt);
  auto playerKin = registry.get<CoKinematics>(pEnt);
  auto newTorpedo = registry.create();
  registry.emplace<CoTorpedo>(newTorpedo);
  registry.emplace<CoItem>(newTorpedo, glm::vec2{0.004, 0.0008}, playerItem.pos,
                          playerItem.rotation);
  registry.emplace<CoKinematics>(
      newTorpedo, 0.6,
      playerKin.vel +
          glm::vec2{cos(playerItem.rotation), sin(playerItem.rotation)} * 0.8f,
      playerKin.angVel);
  registry.emplace<entt::tag<"tex_torpedo"_hs>>(newTorpedo);
  registry.emplace<CoDeletedOoB>(newTorpedo);
  registry.emplace<CoPlayerAligned>(newTorpedo, pEnt);
  registry.emplace<CoTarget>(newTorpedo);
  registry.emplace<CoHasTracer>(newTorpedo);
  torpedoCooldown(pEnt) = cheatMode ? 0.005 : 0.02;

  torpedosLoaded(pEnt) -= cheatMode ? 0.0 : 1.0;
  torpedosFired(pEnt)++;
}

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

        registry.view<CoTarget, CoItem>().each([&](auto target,
                                                   auto &target_item) {
          if (playerAligned.player == target)
            return;
          if (registry.all_of<CoPlayerAligned>(target) &&
              registry.get<CoPlayerAligned>(target).player ==
                  playerAligned.player)
            return;

          float distance = length(item.pos - target_item.pos);
          float angle = glm::dot({cos(item.rotation), sin(item.rotation)},
                                 (target_item.pos - item.pos) / distance);
          float score = angle / distance / distance *
                        (target_item.size.x + target_item.size.y) * 20000.0f;

          float explosionRadiusModifier =
              registry.all_of<CoPlayer>(target) ? 0.5f : 1.5f;

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

        if (torpedo.age < 0.01f) {
          kin.force = glm::vec2(cos(item.rotation), sin(item.rotation)) * 4.0f;
        } else if (torpedo.age < 0.4f) {
          kin.angVel = glm::sign(bestAngle) * 8.0;
          if (bestTarget != entt::null)
            kin.force =
                glm::vec2(cos(item.rotation), sin(item.rotation)) * 4.0f;
          else
            kin.force =
                glm::vec2(cos(item.rotation), sin(item.rotation)) * 2.0f;
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
