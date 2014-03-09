#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include "ulhash3.h"
#include <time.h>
#include <cstdio>

#include "stateManager.h"
#include "bealSearcher.h"

int main(int argc, char** argv) {
  //google::dense_hash_map<int, int> dmap;

  if (argc < 4) {
    fprintf(stderr, "Usage: <from x> <to x> <max power> <max base>\n", argv[0]);
    exit(1);
  }

  int fromX = atol(argv[1]);
  int toX = atol(argv[2]);

  int maxBase = atol(argv[3]);
  int maxPow = atol(argv[4]);

  StateManager stateManager;
  SavedState defaultState(2, fromX, fromX, toX);
  SavedState state = stateManager.load(defaultState);

  std::ofstream logStream;
  logStream.open("logfile.txt", std::ios::app);

  logStream << "x_from=" << state.x_from << std::endl;
  logStream << "x_to=" << state.x_to << std::endl;

  BealSearcher bealSearcher;
  Logger logger;

  std::cout << "Generating z^s..." << std::endl;
  auto hashtables = bealSearcher.genZs(2, 3, maxBase, maxPow);
    
  std::cout << "Done. Searching for candidates..." << std::endl;
  bealSearcher.checkSums(state.x, state.x_to, maxPow, hashtables);

  //std::get<0>(hashtables).free();
  //std::get<1>(hashtables).free();

  //getchar();
}