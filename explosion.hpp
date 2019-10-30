

void UbootGlApp::processExplosions() {
  float explosionDiam = 0.05;
  registry.view<CoExplosion, CoItem, CoAnimated>().each(
      [&](auto entity, auto &explosion, auto &expItem, auto &animation) {
        registry.view<CoTarget, CoItem>().less([&](auto &targetItem) {
          if (glm::length(expItem.pos - targetItem.pos) <
                  explosionDiam * 0.5 +
                      (targetItem.size.x + targetItem.size.y) * 0.5 &&
              explosion.age < 0.08f) {
          }
        });

        explosion.age += simTime;
        animation.frame = explosion.age * (8.0f * 20.0f);

        if (explosion.age > 0.1)
          registry.destroy(entity);
      });
  /*
  for (auto &exp : explosions) {
    for (auto &ag : swarm.agents) {
      if (length(exp.pos - ag.pos) < explosionDiam * 0.9 && exp.age < 0.08f)
        ag.pos = glm::vec2(-1, -1);
    }
    for (int pid = 0; pid < (int)players.size(); pid++) {
      if (length(exp.pos - playerShips[pid].pos) <
              explosionDiam * 0.5 +
                  (playerShips[pid].size.x + playerShips[pid].size.y) * 0.5 &&
          exp.age < 0.08f && exp.player != pid &&
          players[pid].deathtimer == 0.0f) {

        explosions.push_back({glm::vec2{0.02, 0.02}, 0.01, playerShips[pid].vel,
                              playerShips[pid].pos,
                              rand() % 100 * 100.0f * 2.0f * 3.14f, 0,
                              &(textures[5]), pid});
        explosions.back().age = 0.02f;
        sim.sinks.push_back(glm::vec3(playerShips[pid].pos, 200.0f));

        players[pid].deathtimer = 0.08f;
        players[pid].deaths++;
        players[exp.player].kills++;
      }
    }
  }
  */
}
