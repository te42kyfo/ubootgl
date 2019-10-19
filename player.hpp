#pragma once
#include <iostream>

class Player {
public:
  Player(int id) : id(id){};

  float torpedoCooldown = 0.0;
  float torpedosLoaded = 10.0;
  int deaths = 0;
  int kills = 0;
  int torpedosFired = 0;
  int id = -1;

};
