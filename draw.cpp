#include "controls.hpp"
#include "dtime.hpp"
#include "gl_error.hpp"
#include "ubootgl_app.hpp"
#include <GL/glew.h>
#include <glm/gtx/transform.hpp>
#include <glm/vec2.hpp>
#include "implot/implot.h"

using namespace std;

void UbootGlApp::draw() {

  GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

  int xsplits = 1;
  int ysplits = 1;
  if (registry.size<CoPlayer>() == 4) {
    xsplits = 2;
    ysplits = 2;
  } else {
    xsplits = max(1, (int)registry.size<CoPlayer>());
  }

  int displayWidth = ImGui::GetIO().DisplaySize.x;
  int displayHeight = ImGui::GetIO().DisplaySize.y;
  int renderWidth = displayWidth / xsplits;
  int renderHeight = (displayHeight) / ysplits;
  int renderOriginX = renderWidth;
  int renderOriginY = 0;

  bool p_open;

  double graphicsT1 = dtime();

  DrawTracersCS::updatePlayerTracers(registry);


  DrawTracersCS::updateTracers(VelocityTextures::getVXYTex(),
                               VelocityTextures::getFlagTex(), VelocityTextures::getNX(),
                               VelocityTextures::getNY(), gameTimeStep, sim.pwidth);

  ImU32 colors[] = {ImColor(100, 0, 0, 155), ImColor(0, 100, 0, 155),
                      ImColor(100, 100, 0, 155), ImColor(100, 0, 100, 155)};

  registry.view<CoPlayer, CoItem>().each([&](auto &player, auto &item) {
    renderOriginX = (renderWidth + 5) * (player.keySet % xsplits);
    renderOriginY = (renderHeight + 5) * (player.keySet / xsplits);

    ImGui::SetNextWindowPos(
        ImVec2(renderOriginX + 2,
               displayHeight - (player.keySet / xsplits + 1) *
                                   ((displayHeight - 5) / ysplits)));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, colors[player.keySet]);
    ImGui::SetNextWindowSize(ImVec2(200, 120));
    ImGui::Begin(("Player" + to_string(player.keySet)).c_str(), &p_open,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    ImGui::Text("Player %d \n %s", player.keySet,
                getControlString(player.keySet).c_str());
    ImGui::Separator();
    ImGui::Text("Kills: %d, Deaths: %d", player.kills, player.deaths);
    ImGui::Text("Torpedos: %.0f", player.torpedosLoaded);
    if (player.state == PLAYER_STATE::ALIVE_PROTECTED) {
      ImGui::Text("PROTECTED %.2f", player.timer);
    } else if (player.state == PLAYER_STATE::RESPAWNING) {
      ImGui::Text("RESPAWNING %.2f", player.timer);
    }
    ImGui::End();
    ImGui::PopStyleColor();

    GL_CALL(
        glViewport(renderOriginX, renderOriginY, renderWidth, renderHeight));
    GL_CALL(glEnable(GL_FRAMEBUFFER_SRGB));

    GL_CALL(glEnable(GL_BLEND));
    GL_CALL(glBlendFunc(GL_SRC_ALPHA, GL_ONE));

    // Projection-View-Matrix
    glm::mat4 PVM(1.0f);
    // Projection

    PVM = glm::scale(
        PVM, glm::vec3(2 * scale * xsplits,
                       2 * scale * xsplits * (float)renderWidth / renderHeight,
                       1.0f));
    // View
    // PVM = glm::rotate(PVM, playerShips[i].rotation - glm::half_pi<float>(),
    //                  glm::vec3(0.0f, 0.0f, -1.0f));
    PVM = glm::translate(PVM, glm::vec3(-item.pos, 0.0f));

    Draw2DBuf::draw_mag(VelocityTextures::getMagTex(), VelocityTextures::getNX(),
                        VelocityTextures::getNY(), PVM, sim.pwidth);
    Draw2DBuf::draw_buf(sim.p, PVM, sim.pwidth);
    Draw2DBuf::draw_flag(rock_texture, VelocityTextures::getFlagTex(),
                         sim.width, sim.height, PVM, sim.pwidth, texture_offset);

    DrawTracersCS::draw(PVM);
    DrawTracersCS::drawPlayerTracers(PVM);

    DrawFloatingItems::draw(
        registry, entt::type_hash<entt::tag<"tex_debris"_hs>>::value(),
        textures[entt::type_hash<entt::tag<"tex_debris"_hs>>::value()], PVM, 1.0f, true);

    DrawFloatingItems::draw(
        registry, entt::type_hash<entt::tag<"tex_debris1"_hs>>(),
        textures[entt::type_hash<entt::tag<"tex_debris1"_hs>>::value()], PVM, 1.0f,
        true);

    DrawFloatingItems::draw(
        registry, entt::type_hash<entt::tag<"tex_agent"_hs>>::value(),
        textures[entt::type_hash<entt::tag<"tex_agent"_hs>>::value()], PVM, 1.0f, false);

    DrawFloatingItems::draw(
        registry, entt::type_hash<entt::tag<"tex_torpedo"_hs>>::value(),
        textures[entt::type_hash<entt::tag<"tex_torpedo"_hs>>::value()], PVM, 1.0f,
        false, true);
    DrawFloatingItems::draw(
        registry, entt::type_hash<entt::tag<"tex_torpedo"_hs>>::value(),
        textures[entt::type_hash<entt::tag<"tex_torpedo"_hs>>::value()], PVM, 1.0f,
        false);
    DrawFloatingItems::draw(registry, entt::type_hash<entt::tag<"tex_ship"_hs>>::value(),
                            textures[entt::type_hash<entt::tag<"tex_ship"_hs>>::value()],
                            PVM, 1.0f, false, true);

    DrawFloatingItems::draw(registry, entt::type_hash<entt::tag<"tex_ship"_hs>>::value(),
                            textures[entt::type_hash<entt::tag<"tex_ship"_hs>>::value()],
                            PVM, 1.0f, false);

    /*DrawFloatingItems::draw(
        registry, entt::type_hash<entt::tag<"tex_explosion"_hs>>::value(),
        textures[entt::type_hash<entt::tag<"tex_explosion"_hs>>::value()], PVM, 1.0f,
        true, true);*/
    DrawFloatingItems::draw(
        registry, entt::type_hash<entt::tag<"tex_explosion"_hs>>::value(),
        textures[entt::type_hash<entt::tag<"tex_explosion"_hs>>::value()], PVM, 1.0f,
        true);
  });

  renderOriginX = displayWidth * 0.4;
  renderOriginY = displayHeight * 0.0;
  renderWidth = displayWidth * 0.2;
  renderHeight = displayWidth * 0.2;

  GL_CALL(glViewport(renderOriginX, renderOriginY, renderWidth, renderHeight));

  GL_CALL(glBlendFunc(GL_ONE, GL_ZERO));

  // Projection-View-Matrix
  glm::mat4 PVM(1.0f);
  PVM = glm::scale(
      PVM,
      glm::vec3(1.0f, 1.0f * (float)renderWidth / renderHeight, 1.0f) * 2.0f);
  PVM = glm::translate(PVM, glm::vec3(-0.5f, -0.5f, 0.0f));
  Draw2DBuf::draw_flag(black_texture, VelocityTextures::getFlagTex(), sim.width,
                       sim.height, PVM, sim.pwidth);

  DrawFloatingItems::draw(registry, entt::type_hash<entt::tag<"tex_ship"_hs>>::value(),
                          textures[entt::type_hash<entt::tag<"tex_ship"_hs>>::value()],
                          PVM, 5.0f, false, true);
  DrawFloatingItems::draw(registry, entt::type_hash<entt::tag<"tex_agent"_hs>>::value(),
                          textures[entt::type_hash<entt::tag<"tex_agent"_hs>>::value()],
                          PVM, 3.0f, false, true);
  DrawFloatingItems::draw(
      registry, entt::type_hash<entt::tag<"tex_torpedo"_hs>>::value(),
      textures[entt::type_hash<entt::tag<"tex_torpedo"_hs>>::value()], PVM, 3.0f, false,
      true);

  ImGui::SetNextWindowPos(ImVec2(displayWidth - 610, 0));
  ImGui::SetNextWindowSize(ImVec2(600, 120));
  ImGui::Begin("SideBar", &p_open,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

  ImGui::TextWrapped(
      "Total: %5.1f, %5.1f, %5.1f \n  GFX: %5.1f, %5.1f, %5.1f\n  SIM: "
      "%5.1f, %5.1f, %5.1f",
      frameTimes.avg(), frameTimes.high1pct(), frameTimes.largest(),
      gfxTimes.avg(), gfxTimes.high1pct(), gfxTimes.largest(), simTimes.avg(),
      simTimes.high1pct(), simTimes.largest());



  ImGui::SameLine();
  ImPlot::SetNextPlotLimits(0, frameTimes.data().size(), 0, max(simTimes.largest(), frameTimes.largest()), ImGuiCond_Always);
  ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(5, 5));
  ImPlot::BeginPlot("##frameTimePlot", NULL, "ms", ImVec2(450, 105), ImPlotFlags_CanvasOnly, ImPlotAxisFlags_NoDecorations);
  ImPlot::SetNextLineStyle(ImVec4(1.0, 0.2, 0.2, 1.0));
  ImPlot::PlotLine("", frameTimes.data().data(), frameTimes.data().size());
  ImPlot::SetNextLineStyle(ImVec4(0.2, 1.0, 0.2, 1.0));
  ImPlot::PlotLine("", gfxTimes.data().data(), gfxTimes.data().size());
  ImPlot::SetNextLineStyle(ImVec4(0.2, 0.2, 1.0, 1.0));
  ImPlot::PlotLine("", simTimes.data().data(), simTimes.data().size());
  float  data[] = {16.6};
  ImPlot::SetNextLineStyle(ImVec4(1.0, 1.0, 0.2, 1.0));
  ImPlot::PlotHLines("", data, 1);

  ImPlot::EndPlot();
  ImPlot::PopStyleVar();
  ImGui::End();

  double graphicsT2 = dtime();
  gfxTimes.add((graphicsT2 - graphicsT1) * 1000.0);
}
