#include <cmath>
#include <iostream>
#include "../db2dgrid.hpp"
#include "../pressure_solver.hpp"

using namespace std;

int main(int argc, char** argv) {
  int N = 1025;
  float h = 1.0 / (N - 1);
  Single2DGrid u(N, N), rhs(N, N), flag(N, N), r(N, N), reference(N, N);
  r = 0.0;
  u = 0.0;
  rhs = 0.0;
  flag = 1.0;

  for (int y = 0; y < N; y++) {
    u(0, y) = 0;
    u(N - 1, y) = 0;
  }

  for (int x = 1; x < N - 1; x++) {
    u(x, 0) = sinh(0.0) * sin(x / (N - 1.0) * M_PI);
    u(x, N - 1) = sinh(1.0 * M_PI) * sin(x / (N - 1.0) * M_PI);
  }

  for (int y = 0; y < N; y++) {
    for (int x = 1; x < N - 1; x++) {
      reference(x, y) = sinh(y * h * M_PI) * sin(x * h * M_PI);
    }
  }

  cout << calculateResidualField(u, rhs, flag, r, h) << "\n";

  for (int i = 0; i < 5; i++) {
    //rbgs(u, rhs, flag, h, 1.0);
    mg(u, rhs, flag, h);
    cout << calculateResidualField(u, rhs, flag, r, h) << "\n";
  }

  Single2DGrid error(N, N);

  float avgError = 0.0;
  for (int y = 0; y < N; y++) {
    for (int x = 0; x < N; x++) {
      error(x, y) = (reference(x, y) - u(x, y));
      avgError += error(x,y)*error(x,y);
    }
  }
  cout << sqrt(avgError) / N /N << "\n";

  /*  u.print();
  cout << "\n";
  reference.print();
  cout << "\n";
  error.print();
  cout << "\n";
  r.print();*/
}
