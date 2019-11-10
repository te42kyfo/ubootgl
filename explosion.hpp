
void UbootGlApp::newExplosion(glm::vec2 pos, float explosionDiam,
                              entt::entity player, int fragmentLevel) {

  static auto randRotationDist =
      std::uniform_real_distribution<float>(0.0f, 2.0f * 3.141f);
  static auto dist = std::uniform_real_distribution<float>(0.0f, 1.0f);
  static std::default_random_engine gen(std::random_device{}());

  auto newExp = registry.create();
  registry.assign<CoExplosion>(newExp, explosionDiam, fragmentLevel);
  registry.assign<CoItem>(newExp,
                          glm::vec2{explosionDiam, explosionDiam} * 3.0f, pos,
                          randRotationDist(gen));
  if (player != entt::null)
    registry.assign<CoPlayerAligned>(newExp, player);
  registry.assign<entt::tag<"tex_explosion"_hs>>(newExp);
  registry.assign<CoAnimated>(newExp, 0.0f);

  float diam = explosionDiam / sim.h;
  for (int y = -diam; y <= diam; y++) {
    for (int x = -diam; x <= diam; x++) {
      auto gridC = (pos + glm::vec2(x, y) * sim.h) / sim.h + 0.5f;
      if (x * x + y * y > diam * diam || gridC.x < 1 ||
          gridC.x > sim.width - 1 || gridC.y < 1 || gridC.y > sim.height - 1)
        continue;
      if (explosionDiam > 0.002)
        sim.sinks.push_back(glm::vec3(gridC * sim.h, 40.0f));
      if (sim.flag(gridC) < 1.0) {
        for (int i = 0; i < 24; i++) {
          float velangle = randRotationDist(gen);
          float size = dist(gen) * 0.8 + 1.0;
          size *= size;
          int type = (int)(dist(gen) * 2.0f);

          auto newDebris = registry.create();
          registry.assign<CoItem>(
              newDebris, glm::vec2{0.0003, 0.0003} * size,
              pos + glm::vec2(x + dist(gen), y + dist(gen)) * sim.h,
              randRotationDist(gen));

          if (type == 0)
            registry.assign<entt::tag<"tex_debris1"_hs>>(newDebris);
          else
            registry.assign<entt::tag<"tex_debris2"_hs>>(newDebris);
          registry.assign<CoKinematics>(
              newDebris, size * (0.2f * type + 0.2f),
              glm::normalize(glm::vec2(x, y)) * 1.0f +
                  glm::vec2{cos(velangle), sin(velangle)} * dist(gen) * 0.4f,
              dist(gen) - 0.5f);
          registry.assign<CoDeletedOoB>(newDebris);
          registry.assign<CoDecays>(newDebris, 0.81f);
        }
        sim.setGrids(gridC, 1.0);
      }
    }
  }
  sim.mg.updateFields(sim.flag);
}

void UbootGlApp::processExplosions() {

  auto explosionView = registry.view<CoExplosion>();
  auto targetView = registry.view<CoTarget>();

  for (auto it = std::begin(targetView); it != std::end(targetView); it++) {
    auto targetEnt = *it;
    for (auto expIter = std::begin(explosionView);
         expIter != std::end(explosionView); expIter++) {
      auto expEnt = *expIter;

      if (glm::length(pos(expEnt) - pos(targetEnt)) <
          explosionDiam(expEnt) * 0.5 +
              (size(targetEnt).x + size(targetEnt).y) * 0.5) {
        if (registry.has<CoPlayerAligned>(targetEnt) &&
            registry.has<CoPlayerAligned>(expEnt) &&
            registry.get<CoPlayerAligned>(targetEnt).player ==
                registry.get<CoPlayerAligned>(expEnt).player) {
          continue;
        }

        if (registry.has<CoAgent>(targetEnt)) {
          // newExplosion(pos(targetEnt), 0.008f, entt::null);
          pos(targetEnt) = glm::vec2(-1, -1);
        } else if (registry.has<CoTorpedo>(targetEnt)) {
          bumpCount(targetEnt) += 1;
        } else if (registry.has<CoPlayer>(targetEnt)) {
          auto &targetPlayer = registry.get<CoPlayer>(targetEnt);
          if (targetPlayer.deathtimer > 0)
            continue;

          newExplosion(pos(targetEnt), 0.015f, targetEnt);

          targetPlayer.deathtimer = 0.2f;
          targetPlayer.deaths++;

          registry.remove<entt::tag<"tex_ship"_hs>>(targetEnt);

          if (registry.has<CoPlayerAligned>(expEnt))
            registry.get<CoPlayer>(registry.get<CoPlayerAligned>(expEnt).player)
                .kills++;
        }
      }
    }
  }
  registry.view<CoExplosion>().each([&](auto expEnt, auto &exp) {
    exp.age += simTime;
    frame(expEnt) = exp.age * 210.1f + 2; // / exp.explosionDiam * 1.5 + 0.5;

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

        newExplosion(pos(expEnt) + v * explosionDiam(expEnt),
                     explosionDiam(expEnt) * 0.8f,
                     registry.get<CoPlayerAligned>(expEnt).player,
                     fragmentLevel(expEnt) + 1);
      }
    }

    if (age(expEnt) > 0.08) // 10.0f * explosionDiam(expEnt))
      registry.destroy(expEnt);
  });
}
