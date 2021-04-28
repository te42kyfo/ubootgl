#include "ubootgl_app.hpp"
#include <iostream>
#include <omp.h>
#include "dtime.hpp"

using namespace std;


void UbootGlApp::sim_loop(void) {


  int threadCount = 0;
#pragma omp parallel
  {
#pragma omp master
      threadCount = omp_get_num_threads();
  }
  int simulationThreads = max(1, threadCount / 2 - 1);
  cout << simulationThreads << "/" << threadCount << " threads\n";
  omp_set_num_threads(simulationThreads);
  double tprev = dtime();
  double smoothedSimTime = 0.0;
  while (gameRunning) {

    simTimeStep = 0.1f * min(0.2, smoothedSimTime);
    sim.step(simTimeStep);

    double tnow = dtime();
    double dt = tnow - tprev;
    simTimes.add(dt * 1000.f);
    tprev = tnow;
    smoothedSimTime = smoothedSimTime * 0.95 + 0.05 * dt;

    static int frameCounter = 0;
    frameCounter++;
    if (frameCounter % 10 == 0) {
      shiftMap();
    }
  }
  std::cout << " returning \n";
}
