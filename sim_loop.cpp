#include "ubootgl_app.hpp"
#include <iostream>
#include <omp.h>
#include "dtime.hpp"
 #include <unistd.h>

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

  double lastMapShift = dtime();
  while (gameRunning) {

    simTimeStep = 0.1f * min(0.2, smoothedSimTime);
    sim.step(simTimeStep);

    double tnow = dtime();
    // Limit simulation rate to 100fps
    if( tnow - tprev < 0.01) {
      usleep( 1000000 * (0.01 - (tnow-tprev)) );
    }

    tnow = dtime();
    double dt = tnow - tprev;

    simTimes.add(dt * 1000.f);
    tprev = tnow;
    smoothedSimTime = smoothedSimTime * 0.95 + 0.05 * dt;

    static int frameCounter = 0;
    frameCounter++;
    if (  (dtime() - lastMapShift) > 0.4f*terrain.scale) {
      shiftMap();
      lastMapShift = dtime();
    }
  }
  std::cout << " returning \n";
}
