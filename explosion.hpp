
void UbootGlApp::newExplosion(glm::vec2 pos, float explosionDiam,
                              entt::entity player) {
  static auto randRotationDist =
      std::uniform_real_distribution<float>(0.0f, 2.0f * 3.141f);
  static auto dist = std::uniform_real_distribution<float>(0.0f, 1.0f);
  static std::default_random_engine gen(std::random_device{}());

  auto newExplosion = registry.create();
  registry.assign<CoExplosion>(newExplosion, explosionDiam);
  registry.assign<CoItem>(newExplosion, glm::vec2{explosionDiam, explosionDiam}*3.0f,
                          pos, randRotationDist(gen));
  registry.assign<CoPlayerAligned>(newExplosion, player);
  registry.assign<entt::tag<"tex_explosion"_hs>>(newExplosion);
  registry.assign<CoAnimated>(newExplosion, 0.0f);

  float diam = explosionDiam / sim.h;
  for (int y = -diam; y <= diam; y++) {
    for (int x = -diam; x <= diam; x++) {
      auto gridC = (pos + glm::vec2(x, y) * sim.h) / sim.h + 0.5f;
      if (x * x + y * y > diam * diam)
        continue;
      sim.sinks.push_back(glm::vec3(gridC * sim.h, 500.0f));
      if (sim.flag(gridC) < 1.0) {
        for (int i = 0; i < 100; i++) {
          float velangle = randRotationDist(gen);
          float size = dist(gen) * 1.2 + 0.4;
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
              newDebris, size * (0.8f * type + 0.2f),
              glm::normalize(glm::vec2(x, y)) * 1.0f +
                  glm::vec2{cos(velangle), sin(velangle)} * dist(gen) * 0.4f,
              dist(gen) - 0.5f);
          registry.assign<CoDeletedOoB>(newDebris);
          registry.assign<CoTarget>(newDebris);
          registry.assign<CoDecays>(newDebris, 0.25f);
        }
      }
      sim.setGrids(gridC, 1.0);
    }
  }
  sim.mg.updateFields(sim.flag);
}

void UbootGlApp::processExplosions() {

  registry.view<CoExplosion, CoItem, CoAnimated>().each(
      [&](auto entity, auto &explosion, auto &expItem, auto &animation) {
        registry.view<CoTarget, CoItem, CoKinematics>().less(
            [&](auto targetEntity, auto &targetItem, auto &targetKin) {
              if (glm::length(expItem.pos - targetItem.pos) <
                  explosion.explosionDiam * 0.5 +
                      (targetItem.size.x + targetItem.size.y) * 0.5) {
                if (registry.has<CoPlayerAligned>(targetEntity) &&
                    registry.has<CoPlayerAligned>(entity) &&
                    registry.get<CoPlayerAligned>(targetEntity).player ==
                        registry.get<CoPlayerAligned>(entity).player) {
                  return;
                }

                if (registry.has<CoAgent>(targetEntity)) {
                  targetItem.pos = glm::vec2(-1, -1);
                  return;
                }
                if (registry.has<CoTorpedo>(targetEntity)) {
                  targetKin.bumpCount += 1;
                  return;
                }
                if (registry.has<CoPlayer>(targetEntity)) {
                  auto &targetPlayer = registry.get<CoPlayer>(targetEntity);
                  if (targetPlayer.deathtimer > 0)
                    return;

                  newExplosion(targetItem.pos, 0.01f, targetEntity);


                  targetPlayer.deathtimer = 0.08f;
                  targetPlayer.deaths++;

                  if (registry.has<CoPlayerAligned>(entity))
                    registry
                        .get<CoPlayer>(
                            registry.get<CoPlayerAligned>(entity).player)
                        .kills++;
                  return;
                }
              }
            });


        explosion.age += simTime;
        animation.frame = explosion.age / explosion.explosionDiam  * 1.5  + 0.5;

        if (explosion.age > 10.0f * explosion.explosionDiam)
          registry.destroy(entity);
      });
}
