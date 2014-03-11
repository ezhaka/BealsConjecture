#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include "ulhash3.h"
#include <time.h>
#include <cstdio>
#include <math.h>

#include "stateManager.h"
#include "bealSearcher.h"


int main(int argc, char** argv) {
  //google::dense_hash_map<int, int> dmap;

  if (argc < 2) {
    fprintf(stderr, "Usage: <max base> <max power>\n", argv[0]);
    exit(1);
  }

  int maxBase = atol(argv[1]);
  int maxPow = atol(argv[2]);

  MPI::Status stat;
  MPI::Init (argc, argv);

  int size = MPI::COMM_WORLD.Get_size();
  int rank = MPI::COMM_WORLD.Get_rank();

  double q = 0.7;
  int b1 = 20000;
  int prevSum = b1 * (pow(q, rank) - 1) / (q - 1);
  int curr = prevSum + b1 * pow(q, rank);

  int fromX = prevSum;
  int toX = curr;

  std::cout << "from = " << prevSum << std::endl;
  std::cout << "to = " << curr << std::endl;

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

  std::cout << rank << " finished!" << std::endl;

  MPI::Finalize ();
  return 0;
}