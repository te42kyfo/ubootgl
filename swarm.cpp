#include "swarm.hpp"

#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/vec2.hpp>
#include <iostream>

using namespace std;

void classicSwarmAI(entt::registry &registry, const Single2DGrid &flag,
                    float h) {

  auto agentView = registry.view<CoAgent, CoItem, CoKinematics>();
  agentView.less([&](auto &item1, auto &kin1) {
    glm::vec2 swarmCenter = {0, 0};
    int swarmCount = 0;
    glm::vec2 rejectionDirection = {};
    glm::vec2 commonDirection = {};

    for (auto ag2 : agentView) {

      auto item2 = agentView.get<CoItem>(ag2);
      auto kin2 = agentView.get<CoKinematics>(ag2);

      float d = length(item1.pos - item2.pos);

      if (d < 20.0f * item1.size.x) {
        swarmCenter += item2.pos; // + kin2.vel * 0.1f;
        commonDirection += kin2.vel;
        swarmCount++;
      }
      if (d < 0.000001f)
        continue;
      if (d < (item1.size.x + item1.size.y)) {
        rejectionDirection += (item1.pos - item2.pos) / (d + item1.size.x);
      }
    }

    glm::vec2 wallAvoidance = {0.0f, 0.0f};
    glm::vec2 lookAheadPos =
        item1.pos +
        glm::vec2(cos(item1.rotation), sin(item1.rotation)) * 7.0f * h;

    lookAheadPos = glm::min({1.0f, 1.0f}, glm::max({0.0f, 0.0f}, lookAheadPos));

    if (flag(lookAheadPos / h + 0.5f) < 1.0f) {
      wallAvoidance =
          -glm::vec2(cos(item1.rotation + 0.1), sin(item1.rotation + 0.1));
    }
    glm::vec2 lookAheadPos2 = item1.pos + kin1.vel * 0.2f;
    lookAheadPos2 =
        glm::min({1.0f, 1.0f}, glm::max({0.0f, 0.0f}, lookAheadPos2));

    if (flag(lookAheadPos2 / h + 0.5f) < 1.0f) {
      wallAvoidance += -kin1.vel;
    }

    /*
    float interceptionTime = 0.0;
    //     (playerDistance / length(abs(ship.vel) + abs(ag1.vel))) * 0.2f;
    glm::vec2 interceptionVector = (ship.pos + ship.vel * interceptionTime -
                                    ag1.pos + ag1.vel * interceptionTime) /
                                   (playerDistance + 0.2f) / playerDistance;
    */
    auto targetDir = normalize(
        glm::vec2(-0.001, 0) + wallAvoidance * 100.0f +
        (swarmCenter / (float)swarmCount - (item1.pos + kin1.vel * 0.1f)) *
            20.0f +
        10.0f * rejectionDirection +
        10.0f * commonDirection / (float)swarmCount);

    auto heading = glm::vec2{cos(item1.rotation), sin(item1.rotation)};
    float directedAngle = glm::orientedAngle(targetDir, heading);

    if (directedAngle < 0)
      item1.rotation += 0.041;
    else
      item1.rotation -= 0.041;

    kin1.force = 2.0f * glm::vec2(cos(item1.rotation), sin(item1.rotation)) *
                 (dot(targetDir, heading) + 0.5f) / 1.5f;
    assert(!std::isnan(kin1.force.x));
    assert(!std::isnan(kin1.force.y));
  });
}

/*
enum class AGENT_ACTION : int { acc, rot_left, rot_right, nop };

void Swarm::nnUpdate(FloatingItem ship, const Single2DGrid &flag, float h) {
    Matrix2D<float, 4, 7> wgrad(0.0);
Matrix2D<float, 4, 1> bgrad(0.0);

float totalReward = 0.0;
for (size_t i = 0; i < agents.size(); i++) {
  auto &ag1 = agents[i];

  auto futurePos =
      (ag1.pos + glm::rotate(glm::vec2(1.0, 0.0), ag1.rotation) * 0.01f) / h +
      glm::vec2{0.5, 0.5};

  Matrix2D<float, 7, 1> input = {ag1.vel.x,
                                 ag1.vel.y,
                                 cos(ag1.rotation),
                                 sin(ag1.rotation),
                                 (float)ag1.bumpCount,
                                 lastActionPotentials[i][0][3][0],
                                 flag(futurePos.x, futurePos.y)};

  auto actionPotential = bias + weights * input;

  std::normal_distribution<float> dist(0.0, 0.5);
  if (dist(gen) > 2.0) {
    actionPotential[0][0] = dist(gen);
    actionPotential[1][0] = dist(gen);
    actionPotential[2][0] = dist(gen);
  }
  if (actionPotential[0][0] > 0) {
    ag1.force = 4.0f * glm::vec2(cos(ag1.rotation), sin(ag1.rotation));
  }
  if (actionPotential[1][0] > 0) {
    if (actionPotential[2][0] > 0)
      ag1.rotation -= 0.1;
    else
      ag1.rotation += 0.1;
  }

  float reward = 0.1; // length(ag1.vel);

  if (ag1.bumpCount > 0) {
    reward = -1000.0;
    ag1.bumpCount = 0;
  }
  totalReward += reward;
  for (uint n = 0; n < lastInputs[i].size(); n++) {
    wgrad = wgrad + lastActionPotentials[i][n] * reward *
                        lastInputs[i][n].transpose();
    bgrad = bgrad + lastActionPotentials[i][n] * reward;
  }

  rotate(begin(lastInputs[i]), begin(lastInputs[i]) + 1, end(lastInputs[i]));
  lastInputs[i][lastInputs[i].size() - 1] = input;

  rotate(begin(lastActionPotentials[i]), begin(lastActionPotentials[i]) + 1,
         end(lastActionPotentials[i]));
  lastActionPotentials[i][lastInputs[i].size() - 1] = actionPotential;
}

wgrad = wgrad * (1.0f / (float)agents.size() / lastInputs.size());
bgrad = bgrad * (1.0f / (float)agents.size() / lastInputs.size());

cout << wgrad << "\n";
cout << weights << "\n";
cout << bgrad << "\n";
cout << bias << "\n";

cout << totalReward / agents.size() << "\n";

wgrad.clamp(-0.2f, 0.2f);
bgrad.clamp(-0.2f, 0.2f);

weights = weights + wgrad * 0.001f;
bias = bias + bgrad * 0.001f;

  //  cout << "\n\n";
  }*/
