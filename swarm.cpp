#include "swarm.hpp"

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/vec2.hpp>
#include <iostream>

using namespace std;

void Swarm::addAgent(FloatingItem agent) { agents.push_back(agent); }

void Swarm::update(FloatingItem ship, const Single2DGrid &flag, float h) {
  for (auto &ag1 : agents) {
    glm::vec2 swarmCenter = {0, 0};
    int swarmCount = 0;
    glm::vec2 rejectionDirection = {};
    glm::vec2 commonDirection = {};
    for (auto &ag2 : agents) {

      float d = length(ag1.pos - ag2.pos);

      if (d < 9.0f * ag1.size.x) {
        swarmCenter += ag2.pos + ag2.vel * 0.1f;
        commonDirection +=
            ag2.vel; // glm::vec2{sin(ag2.rotation), cos(ag2.rotation)};
        swarmCount++;
      }
      if (d < 0.000001f)
        continue;
      if (d < 2.0f * ag1.size.x) {
        rejectionDirection += (ag1.pos - ag2.pos) / (d + ag1.size.x);
      }
    }

    float dxwall = bilinearSample(flag, (ag1.pos + ag1.vel * 0.1f +
                                         glm::vec2{2.0f * ag1.size.x, 0.0f}) /
                                            h) -
                   bilinearSample(flag, (ag1.pos + ag1.vel * 0.1f -
                                         glm::vec2{2.0f * ag1.size.x, 0.0f}) /
                                            h);

    float dywall = bilinearSample(flag, (ag1.pos + ag1.vel * 0.1f +
                                         glm::vec2{0.0f, 2.0f * ag1.size.x}) /
                                            h) -
                   bilinearSample(flag, (ag1.pos + ag1.vel * 0.1f -
                                         glm::vec2{0.0f, 2.0f * ag1.size.x}) /
                                            h);

    glm::vec2 wallAvoidance = {dxwall, dywall};

    glm::vec2 targetDir;
    float playerDistance = length(ship.pos - ag1.pos);

    float interceptionTime =
        (playerDistance / length(abs(ship.vel) + abs(ag1.vel))) * 0.2f;
    glm::vec2 interceptionVector = -(ship.pos + ship.vel * interceptionTime -
                                     ag1.pos + ag1.vel * interceptionTime) /
                                   (playerDistance + 0.2f) / playerDistance;

    targetDir = normalize(
        (swarmCenter / (float)swarmCount - (ag1.pos + ag1.vel * 0.1f)) * 40.0f +
        40.0f * rejectionDirection * 1.0f +
        1.2f * commonDirection / (float)swarmCount + 0.3f * interceptionVector +
        wallAvoidance * 10000.0f);

    auto heading = glm::vec2{cos(ag1.rotation), sin(ag1.rotation)};
    float directedAngle = glm::orientedAngle(targetDir, heading);
    //    float angle = atan2(targetDir.y, targetDir.x);

    if (directedAngle < 0)
      ag1.rotation += 0.1;
    else
      ag1.rotation -= 0.1;

    ag1.force = 4.0f * glm::vec2(cos(ag1.rotation), sin(ag1.rotation)) *
                dot(targetDir, heading);
  }
}
