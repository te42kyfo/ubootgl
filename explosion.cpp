#include "ubootgl_app.hpp"

void UbootGlApp::newExplosion(glm::vec2 pos, float explosionDiam,
                              entt::entity player, int fragmentLevel) {

  static auto randRotationDist =
      std::uniform_real_distribution<float>(0.0f, 2.0f * 3.141f);
  static auto dist = std::uniform_real_distribution<float>(0.0f, 1.0f);
  static std::default_random_engine gen(std::random_device{}());

  auto newExp = registry.create();
  registry.emplace<CoExplosion>(newExp, explosionDiam, fragmentLevel);
  registry.emplace<CoItem>(newExp,
                           glm::vec2{explosionDiam, explosionDiam} * 3.0f, pos,
                           randRotationDist(gen));
  if (player != entt::null)
    registry.emplace<CoPlayerAligned>(newExp, player);
  registry.emplace<entt::tag<"tex_explosion"_hs>>(newExp);
  registry.emplace<CoAnimated>(newExp, 0.0f);

  float diam = explosionDiam * 0.8f / sim.h;
  for (int y = -diam; y <= diam; y++) {
    for (int x = -diam; x <= diam; x++) {
      auto gridC = (pos + glm::vec2(x, y) * sim.h) / sim.h + 0.5f;
      if (x * x + y * y > diam * diam || gridC.x < 0 ||
          gridC.x > sim.flag.width || gridC.y < 2 ||
          gridC.y > sim.flag.height - 3)
        continue;
      if (explosionDiam > 0.002)
        sim.sinks.push_back(glm::vec3(gridC * sim.h, 100.0f));
      if (sim.flag(gridC) < 1.0) {
        for (int i = 0; i < 2; i++) {
          float velangle = randRotationDist(gen);
          float size = dist(gen) * 1.2 + 1.1f;
          size *= size;
          int type = (int)(dist(gen) * 2.0f);

          auto newDebris = registry.create();
          registry.emplace<CoItem>(
              newDebris, glm::vec2{0.0003, 0.0003} * size,
              pos + glm::vec2(x + dist(gen), y + dist(gen)) * sim.h,
              randRotationDist(gen));

          registry.emplace<entt::tag<"tex_debris"_hs>>(newDebris);
          registry.emplace<CoAnimated>(newDebris, static_cast<float>(type));
          registry.emplace<CoKinematics>(
              newDebris, 0.3 * size * (0.3f * type + 0.06f),
              glm::vec2(x, y) +
                  glm::vec2{cos(velangle), sin(velangle)} * dist(gen) * 0.4f,
              (randRotationDist(gen) - randRotationDist(gen)) * 1000.0f);
          registry.emplace<CoDeletedOoB>(newDebris);
          registry.emplace<CoDecays>(newDebris, 0.41f);
        }
        sim.setGrids(gridC, 1.0);
      }
    }
  }
}

void UbootGlApp::processExplosions() {

  auto explosionView = registry.view<CoExplosion>();
  auto targetView = registry.view<CoTarget>();

  for (auto it = std::begin(targetView); it != std::end(targetView); it++) {
    auto targetEnt = *it;
    float explosionRadiusModifier =
        registry.all_of<CoPlayer>(targetEnt) ? 0.7f : 1.5f;
    for (auto expIter = std::begin(explosionView);
         expIter != std::end(explosionView); expIter++) {
      auto expEnt = *expIter;

      if (registry.get<CoExplosion>(expEnt).age < 0.002f)
        continue;

      if (glm::length(pos(expEnt) - pos(targetEnt)) <
          (explosionDiam(expEnt) * 0.5 +
           (size(targetEnt).x + size(targetEnt).y) * 0.5) *
              explosionRadiusModifier) {
        auto targetAligned = registry.try_get<CoPlayerAligned>(targetEnt);
        auto expAligned = registry.try_get<CoPlayerAligned>(expEnt);
        if (targetAligned && expAligned &&
            targetAligned->player == expAligned->player) {
          continue;
        }

        if (registry.all_of<CoAgent>(targetEnt)) {

          newExplosion(pos(targetEnt), 0.008f,
                       expAligned ? expAligned->player : entt::null);

          pos(targetEnt) = glm::vec2(-1, -1);

        } else if (registry.all_of<CoTorpedo>(targetEnt)) {
          bumpCount(targetEnt) += 1;
        } else if (registry.all_of<CoPlayer>(targetEnt)) {
          auto &targetPlayer = registry.get<CoPlayer>(targetEnt);
          if (targetPlayer.state != PLAYER_STATE::ALIVE)
            continue;

          if (expAligned)
            newExplosion(pos(targetEnt), 0.01f,
                         registry.get<CoPlayerAligned>(expEnt).player);
          else
            newExplosion(pos(targetEnt), 0.01f, entt::null);

          targetPlayer.state = PLAYER_STATE::KILLED;

          if (expAligned)
            registry.get<CoPlayer>(expAligned->player).kills++;
        }
      }
    }
  }
  registry.view<CoExplosion>().each([&](auto expEnt, auto &exp) {
    exp.age += gameTimeStep;
    frame(expEnt) =
        exp.age * 180.0f; //*210.1f; // / exp.explosionDiam * 1.5 + 0.5;

    auto expAligned = registry.try_get<CoPlayerAligned>(expEnt);
    auto playerEnt = expAligned ? expAligned->player : entt::null;

    if (exp.explosionDiam > 0.0002 && exp.age > 0.003 && !exp.fragmented) {
      exp.fragmented = true;
      static std::default_random_engine gen(std::random_device{}());
      static auto dist = std::uniform_real_distribution<float>(-1.0f, 1.0f);
      int count = 1;
      if (exp.fragmentLevel <= 2)
        count = 2;

      for (int i = 0; i < count; i++) {
        auto v = glm::vec2(dist(gen), dist(gen));
        while (glm::length(v) > 1.0)
          v = glm::vec2(dist(gen), dist(gen));

        newExplosion(pos(expEnt) + v * explosionDiam(expEnt) * 1.0f,
                     explosionDiam(expEnt) * 0.6f, playerEnt,
                     fragmentLevel(expEnt) + 1);
      }
    }

    if (age(expEnt) > 0.07) // 10.0f * explosionDiam(expEnt))
      registry.destroy(expEnt);
  });

  sim.mg.updateFields(sim.flag);
  VelocityTextures::uploadFlag(sim.getFlag());
}
