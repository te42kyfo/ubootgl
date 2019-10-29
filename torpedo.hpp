#include "floating_item.hpp"
#include <glm/gtx/vector_angle.hpp>

void UbootGlApp::processTorpedos() {
  float explosionDiam = 0.005;
  registry.view<CoTorpedo, CoItem, CoKinematics, CoPlayerAligned>().each(
      [&](auto entity, auto &torpedo, auto &item, auto &kin,
          auto &playerAligned) {
        torpedo.age += simTime;
        entt::entity bestTarget = entt::null;
        float bestScore = 7.0;
        float bestAngle = 0.0;

        bool explodes = false;

        registry.view<CoTarget, CoItem>().less(
            [&](auto target, auto &target_item) {
              if (playerAligned.player == target)
                return;

              float distance = length(item.pos - target_item.pos);
              float angle = glm::dot({cos(item.rotation), sin(item.rotation)},
                                     (target_item.pos - item.pos) / distance);
              float score = angle * angle / distance / distance *
                            target_item.size.x * target_item.size.y * 20000.0f;

              if (distance < (target_item.size.x + target_item.size.y) * 0.6f)
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
          kin.force = glm::vec2(cos(item.rotation), sin(item.rotation)) * 2.0f;
        } else if (torpedo.age < 0.8f) {
          kin.angVel = glm::sign(bestAngle) * 10.0;
          if (bestTarget != entt::null)
            kin.force =
                glm::vec2(cos(item.rotation), sin(item.rotation)) * 4.0f;
          else
            kin.force =
                glm::vec2(cos(item.rotation), sin(item.rotation)) * 0.0f;
        } else {
          explodes = true;
        }
        if (kin.bumpCount > 0)
          explodes = true;

        if (explodes) {

          static auto randRotationDist =
              std::uniform_real_distribution<float>(0.0f, 2.0f * 3.141f);
          static auto dist = std::uniform_real_distribution<float>(0.0f, 1.0f);
          static std::default_random_engine gen(std::random_device{}());

          auto newExplosion = registry.create();
          registry.assign<CoExplosion>(newExplosion, 0.02f);
          registry.assign<entt::tag<"tex_explosion"_hs>>(newExplosion);
          registry.assign<CoItem>(newExplosion, glm::vec2{0.01, 0.01}, item.pos,
                                  randRotationDist(gen));
          registry.assign<CoPlayerAligned>(newExplosion, playerAligned);

          float diam = explosionDiam / sim.h;
          for (int y = -diam; y <= diam; y++) {
            for (int x = -diam; x <= diam; x++) {
              auto gridC = (item.pos + glm::vec2(x, y) * sim.h) / sim.h + 0.5f;
              if (x * x + y * y > diam * diam)
                continue;
              sim.sinks.push_back(glm::vec3(item.pos, 400.0f));
              if (sim.flag(gridC) < 1.0) {
                for (int i = 0; i < 3000; i++) {
                  float velangle =
                      orientedAngle(kin.vel, glm::vec2{0.0f, 1.0f}) +
                      randRotationDist(gen);
                  float size = dist(gen) + 0.2;
                  size *= size;
                  int type = (int)(dist(gen) * 2.0f);

                  auto newDebris = registry.create();
                  registry.assign<CoItem>(
                      newDebris, glm::vec2{0.0003, 0.0003} * size,
                      item.pos +
                          glm::vec2(x + dist(gen), y + dist(gen)) * sim.h,
                      randRotationDist(gen));
                  
                  if (type == 0)
                    registry.assign<entt::tag<"tex_debris1"_hs>>(newDebris);
                  else
                    registry.assign<entt::tag<"tex_debris2"_hs>>(newDebris);
                  registry.assign<CoKinematics>(
                      newDebris, size * (0.8f * type + 0.2f),
                      kin.vel + glm::vec2{cos(velangle), sin(velangle)} *
                                    dist(gen) * 0.4f,
                      dist(gen) - 0.5f);
                  registry.assign<CoDeletedOoB>(newDebris);
                  registry.assign<CoTarget>(newDebris);
                  registry.assign<CoDecays>(newDebris, 10.05f);
                }
              }
              sim.setGrids(gridC, 1.0);
            }
          }
          sim.mg.updateFields(sim.flag);

          registry.destroy(entity);
        }
      });
}
