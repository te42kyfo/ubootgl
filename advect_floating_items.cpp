#include "interpolators.hpp"
#include "simulation.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

using glm::vec2;

// input in standard grid space
vec2 inline Simulation::bilinearVel(vec2 c) {
  return vec2(bilinearSample(vx, c - vec2(0.5f, 0.0f)),
              bilinearSample(vy, c - vec2(0.0f, 0.5f)));
}

void Simulation::advectFloatingItems(entt::registry &registry, float gameDT) {
  auto floatingItemView = registry.view<CoItem, CoKinematics>();
  std::scoped_lock lock(accum_mutex);
  for (auto entity : floatingItemView) {
    auto &item = floatingItemView.get<CoItem>(entity);
    auto &kin = floatingItemView.get<CoKinematics>(entity);

    int iterationSteps =
        fmin(15.0f, fmax(1.0f, glm::max(abs(kin.vel.x), abs(kin.vel.y)) *
                                   gameDT / h * 2.5));

    float subDT = gameDT / iterationSteps;

    for (int step = 0; step < iterationSteps; step++) {
      glm::vec2 posBefore = item.pos;

      // Position update
      item.pos += subDT * kin.vel;
      assert(!std::isnan(item.pos.x) && !std::isnan(item.pos.y));

      item.rotation =
          fmod(item.rotation + subDT * kin.angVel + 2 * M_PI, 2 * M_PI);

      glm::vec2 gridPos = item.pos / (pwidth / (flag.width));

      // Skip out of Bounds objects
      if (gridPos.x >= width - 2 || gridPos.x <= 1.0 ||
          gridPos.y >= height - 2 || gridPos.y <= 1.0)
        // return;
        continue;

      // Check terrain collision
      if (psampleFlagLinear(item.pos) < 0.5f) {
        glm::vec2 surfaceNormal =
            normalize(psampleFlagNormal(0.5f * (posBefore + item.pos)));
        if (psampleFlagLinear(posBefore) > 0.5f)
          item.pos = posBefore;
        if (length(surfaceNormal) > 0.0f) {
          if (dot(kin.vel, surfaceNormal) < 0.0)
            kin.vel = reflect(kin.vel, surfaceNormal) * 0.7f;
          kin.vel += surfaceNormal * 0.001f;
          if (dot(kin.force, surfaceNormal) < 0.0f)
            kin.force += dot(kin.force, surfaceNormal) * surfaceNormal;
        } else {
          kin.vel = vec2(0.0f);
          // kin.force = vec2(0.0f);
        }
        kin.bumpCount++;
      }

      glm::vec2 externalForce = glm::vec2(0.0, -0.5) * kin.mass + kin.force;
      glm::vec2 centralForce = glm::vec2(0.0, 0.0);
      float angForce = kin.angForce;

      glm::vec2 surfacePoints[] = {
          {-1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, -1.0f}, {0.0f, 1.0f}};

      float sideLength[] = {item.size.y, item.size.y, item.size.x, item.size.x};

      for (int i = 0; i < 4; i++) {
        int nSP = fmax(2.0f, sideLength[i] / h);

        for (int n = 0; n < nSP; n++) {
          auto sp = surfacePoints[i] + glm::abs(glm::vec2(surfacePoints[i].y,
                                                          surfacePoints[i].x)) *
                                           (1.0f - n * 2.0f / (nSP - 1));

          auto tSP = sp * item.size * 0.5f;
          tSP = glm::rotate(tSP, item.rotation);
          tSP = tSP + item.pos;

          auto deltaVel =
              bilinearVel(tSP / h) -
              // vec2(bilinearSample(vx_accum, tSP - vec2(0.5f, 0.0f)),
              //     bilinearSample(vy_accum, tSP - vec2(0.0f, 0.5f))) -
              (kin.vel + 3.141f *
                             glm::rotate(sp * item.size * 0.5f,
                                         item.rotation + 0.5f * 3.141f) *
                             kin.angVel);

          vec2 normal = glm::rotate(glm::normalize(sp), item.rotation);
          auto force =
              normal *
              glm::min(0.0f, dot(deltaVel,
                                 glm::rotate(surfacePoints[i], item.rotation)));
          centralForce += force * 400000.0f * (0.003f + sideLength[i]) *
                          sideLength[i] / (float)nSP;

          if (tSP.x / h < 1.0f || tSP.x / h > vx.width - 2.0f ||
              tSP.y / h < 1.0f || tSP.y / h > vx.height - 2.0f)
            continue;
          auto deltaVec = force * (0.003f + sideLength[i]) * sideLength[i] /
                          (float)nSP * subDT * 18000000.0f;
          glm::vec2 cx = tSP / h - vec2(0.5f, 0.0);
          bilinearScatter(vx_accum, cx, -deltaVec.x);
          glm::vec2 cy = tSP / h - vec2(0.0f, 0.5);
          bilinearScatter(vy_accum, cy, -deltaVec.y);
        }
      }

      centralForce += 1000.0f * (item.size[0] + item.size[1]) *
                      (bilinearVel(item.pos / h) - kin.vel);

      // Calculate new velocity
      kin.vel += subDT * (externalForce + centralForce) / kin.mass;
      assert(!std::isnan(kin.vel.x) && !std::isnan(kin.vel.y));

      float angMass = item.size.x * item.size.y * kin.mass * (1.0f / 12.0f);
      kin.angVel += subDT * angForce / angMass;
      // Hacky, non physical decay of angular velocity
      kin.angVel *= 0.98;

      // Compute forces on fluid
      // gridPos = item.pos / (pwidth / (flag.width));
      // if (gridPos.x < 1 || gridPos.x > flag.width - 1 || gridPos.y < 2 ||
      //    gridPos.y > flag.height - 2)
      //  continue;
    }

    kin.angForce = 0;
    kin.force = {0, 0};
  }
}

void Simulation::advectFloatingItemsSimple(entt::registry &registry,
                                           float gameDT) {
  auto floatingItemView = registry.view<CoItem, CoKinematicsSimple>();

  std::vector<std::vector<glm::vec2>> bins(100);

  for (auto ent2 : floatingItemView) {
    auto &item2 = floatingItemView.get<CoItem>(ent2);
    int bin = ((int)(item2.pos.x * 100)) % bins.size();
    bins[bin].push_back(item2.pos);
  }

  std::scoped_lock lock(accum_mutex);

  for (auto entity : floatingItemView) {
    auto &item = floatingItemView.get<CoItem>(entity);
    auto &kin = floatingItemView.get<CoKinematicsSimple>(entity);

    glm::vec2 repForce = {0, 0};
    int contactCount = 0;
    int bin = ((int)(item.pos.x * 100)) % bins.size();
    for (auto pos2 : bins[bin]) {

      auto dis = item.pos - pos2;
      if (dis.x * dis.x + dis.y * dis.y < (item.size.x * item.size.y * 0.4f)) {
        auto len = glm::max(0.1f * item.size.x, length(dis));

        repForce += 0.0001f * (dis / len / len);
        contactCount++;
      }
    }
    repForce /= glm::max(1, contactCount);
    kin.force += repForce * 10.0f;

    int iterationSteps =
        fmin(15.0f, fmax(1.0f, glm::max(abs(kin.vel.x), abs(kin.vel.y)) *
                                   gameDT / h * 2.5));

    float subDT = gameDT / iterationSteps;

    for (int step = 0; step < iterationSteps; step++) {
      glm::vec2 posBefore = item.pos;

      // Position update
      item.pos += subDT * kin.vel;
      assert(!std::isnan(item.pos.x) && !std::isnan(item.pos.y));

      item.rotation =
          fmod(item.rotation + subDT * kin.angVel + 2 * M_PI, 2 * M_PI);

      glm::vec2 gridPos = item.pos / (pwidth / (flag.width));

      // Skip out of Bounds objects
      if (gridPos.x >= width - 2 || gridPos.x <= 1.0 ||
          gridPos.y >= height - 2 || gridPos.y <= 1.0)
        // return;
        continue;

      // Check terrain collision
      if (psampleFlagLinear(item.pos) < 0.5f) {
        glm::vec2 surfaceNormal =
            normalize(psampleFlagNormal(0.5f * (posBefore + item.pos)));
        if (psampleFlagLinear(posBefore) > 0.5f)
          item.pos = posBefore;
        if (length(surfaceNormal) > 0.0f) {
          if (dot(kin.vel, surfaceNormal) < 0.0)
            kin.vel = reflect(kin.vel, surfaceNormal) * 0.7f;
          kin.vel += surfaceNormal * 0.001f;
          if (dot(kin.force, surfaceNormal) < 0.0f)
            kin.force += dot(kin.force, surfaceNormal) *1.0f* surfaceNormal;
          auto lat = glm::rotate(surfaceNormal, 0.5f*(float)M_PI);
          auto latVel = dot(lat, kin.vel);
          auto rotVel = kin.angVel * (item.size.x+item.size.y) * 0.5f;
          auto latDiff = latVel - rotVel;

          //kin.angForce += latDiff / (item.size.x+item.size.y) * 2.0f * 0.000001f;
          kin.force += latDiff * lat * 1.0f;
        } else {
          kin.vel = vec2(0.0f);
          // kin.force = vec2(0.0f);
        }
        kin.bumpCount++;
      }

      glm::vec2 externalForce = glm::vec2(0.0, -0.5) * kin.mass + kin.force;

      vec2 deltaVel = bilinearVel(item.pos / h) - kin.vel;
      externalForce += 1000.0f * (item.size[0] + item.size[1]) * deltaVel;

      if (item.pos.x / h > 1.0f && item.pos.x / h < vx.width - 2.0f &&
          item.pos.y / h < 1.0f && item.pos.y / h < vx.height - 2.0f) {
        auto deltaVec = deltaVel * item.size.x * item.size.y;

        glm::vec2 cx = item.pos / h - vec2(0.5f, 0.0);
        bilinearScatter(vx_accum, cx, -deltaVec.x);
        glm::vec2 cy = item.pos / h - vec2(0.0f, 0.5);
        bilinearScatter(vy_accum, cy, -deltaVec.y);
      }

      // Calculate new velocity
      kin.vel += subDT * externalForce / kin.mass;
      assert(!std::isnan(kin.vel.x) && !std::isnan(kin.vel.y));

      auto iGridPos = glm::ivec2(gridPos);
      float fluidAngVel = -((vx(iGridPos) - vx(iGridPos - glm::ivec2{0, 1})) -
                           (vy(iGridPos) - vy(iGridPos - glm::ivec2{1, 0}))) /
                          h / 2;

      float angMass = item.size.x * item.size.y * kin.mass * (1.0f / 12.0f);
      float k = glm::min(1.0f, subDT / angMass * 0.005f *
                                   (item.size.x + item.size.y) / 4.0f);

      kin.angVel += k * (fluidAngVel - kin.angVel);
      kin.angVel += subDT / angMass * kin.angForce;

      vx_accum(iGridPos) -= kin.angVel * k * 0.01f * p(iGridPos);
      vx_accum(iGridPos - glm::ivec2{0, 1}) +=
          kin.angVel * k * 0.01f * p(iGridPos - glm::ivec2{0, 1});
      vy_accum(iGridPos) += kin.angVel * k * 0.01f * p(iGridPos);
      vy_accum(iGridPos - glm::ivec2{1, 0}) -=
          kin.angVel * k * 0.01f * p(iGridPos - glm::ivec2{1, 0});
    }

    kin.angForce = 0;
    kin.force = {0, 0};
  }
}
