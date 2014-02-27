#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include "ulhash3.h"
//#include <cilk\cilk.h>
#include <time.h>
#include "ulhashHashtable.h"
#include <cstdio>

//#include <sparsehash/internal/sparseconfig.h>
//#include <config.h>
//#include <sparsehash/sparse_hash_set>
//#include <sparsehash/sparse_hash_map>
//#include <sparsehash/dense_hash_set>
//#include <sparsehash/dense_hash_map>
//#include <sparsehash/template_util.h>
//#include <sparsehash/type_traits.h>

unsigned int numCand = 0;

std::ofstream oLogFile;
std::string logFileName = "logfile.txt";
std::tuple<uint64, uint64> readLogFile(int defaultZ, int defaultX);

uint64 largeP1 = 4294967291ULL; //(1<<31) - 5;
uint64 largeP2 = 4294967279ULL; //(1<<31) - 17;

clock_t startClock;
void startTimer();
void stopTimerAndPrint();

uint64 gcd(uint64 u, uint64 v);
void logCurrentTime();
int verbose = 0;
int useGcd = 1;

void checkValues(uint64 x, uint64 m, uint64 y, uint64 n, std::vector<std::tuple<uint64, uint64>> zrs1, std::vector<std::tuple<uint64, uint64>> zrs2);
void checkSums(int fromX, int toX, int maxPow, std::tuple<UlhashHashtable, UlhashHashtable> hashtables);
std::tuple<UlhashHashtable, UlhashHashtable> genZs(uint64 from, uint64 to, uint64 maxBase, uint64 maxPow);

int main(int argc, char** argv) {
  //google::dense_hash_map<int, int> dmap;

  printf("sizeof(uint64) = %d\n", sizeof(uint64));

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <max power> <max base>\n", argv[0]);
    exit(1);
  }

  if (!strcmp(argv[1], "-v")) {
    verbose = 1;
    argv++;
  }

  if (!strcmp(argv[1], "-a")) {
    useGcd = 0;
    argv++;
  }

  int fromX = atol(argv[1]);
  int toX = atol(argv[2]);

  int maxBase = atol(argv[3]);
  int maxPow = atol(argv[4]);

  auto savedState = readLogFile(2, fromX);
  int savedZ = std::get<0>(savedState);
  int savedX = std::get<1>(savedState);

  oLogFile.open(logFileName, std::ios::app);

  int zStep = 10000;
  int fromZ = savedZ == 2 ? 1 : ((savedZ / zStep) + 1);

  for (uint64 z = fromZ; z <= maxBase / zStep; z++)
  {
    auto hashtables = genZs((z - 1) * zStep, z * zStep, maxBase, maxPow);

    printf("Done. Searching for candidates...\n");
    checkSums(z == fromZ ? savedX : fromX, toX, maxPow, hashtables);

    std::get<0>(hashtables).free();
    std::get<1>(hashtables).free();

    printf("Finished. A total of %d candidates were found.\n", numCand);
    oLogFile << "z=" << z * zStep << std::endl << std::flush;
  }
  //getchar();
}

uint64 modularPow(uint64 base, uint64 pow, uint64 modulo)
{
  uint64 result = (base*base) % modulo;

  for (int m = 2; m <= pow; m++) {
    result = (result * base) % modulo;
  }

  return result;
}

void checkValues(uint64 x, uint64 m, uint64 y, uint64 n, std::vector<std::tuple<uint64, uint64>> zrs1, std::vector<std::tuple<uint64, uint64>> zrs2)
{
  for (auto zr1 = zrs1.begin(); zr1 != zrs1.end(); zr1++) {
    auto zr1tuple = *zr1;
    uint64 z = std::get<0>(zr1tuple);

    if (gcd(x, z) == 1 && gcd(y, z) == 1) {
      numCand++;
      printf("%u^%u + %u^%u\n", x, m, y, n);
      oLogFile << x << "^" << m << " + " << y << "^" << n << " = " << z << "^" << std::get<1>(zr1tuple) << std::endl;
    }
  }

  for (auto zr2 = zrs2.begin(); zr2 != zrs2.end(); zr2++) {
    auto zr2tuple = *zr2;
    uint64 z = std::get<0>(zr2tuple);

    if (gcd(x, z) == 1 && gcd(y, z) == 1) {
      numCand++;
      printf("%u^%u + %u^%u\n", x, m, y, n);
      oLogFile << x << "^" << m << " + " << y << "^" << n << " = " << z << "^" << std::get<1>(zr2tuple) << std::endl;
    }
  }
}

// Check each x^m + y^n w/ gcd(m,n)==1 for inclusion in the trie
// Output each as a candidate if it matches.
void checkSums(int fromX, int toX, int maxPow, std::tuple<UlhashHashtable, UlhashHashtable> hashtables) {
  unsigned int x, y, m, n;
  startTimer();

  UlhashHashtable hp1 = std::get<0>(hashtables);
  UlhashHashtable hp2 = std::get<1>(hashtables);

  for (x = fromX; x <= toX; x++) {
    for (y = 2; y <= x; y++) {
      if (useGcd && gcd(x, y) != 1) continue;

      // compute modular powers here
      uint64* pxp1 = new uint64[maxPow - 3 + 1];
      uint64* pxp2 = new uint64[maxPow - 3 + 1];
      uint64* pyp1 = new uint64[maxPow - 3 + 1];
      uint64* pyp2 = new uint64[maxPow - 3 + 1];
      uint64 powxp1 = (x*x) % largeP1;
      uint64 powxp2 = (x*x) % largeP2;
      uint64 powyp1 = (y*y) % largeP1;
      uint64 powyp2 = (y*y) % largeP2;

      for (int m = 2; m <= maxPow; m++) {
        int minPow = 3;

        if (m >= minPow) {
          pxp1[m - minPow] = powxp1;
          pxp2[m - minPow] = powxp2;
          pyp1[m - minPow] = powyp1;
          pyp2[m - minPow] = powyp2;
        }

        powxp1 = (powxp1 * x) % largeP1;
        powxp2 = (powxp2 * x) % largeP2;
        powyp1 = (powyp1 * x) % largeP1;
        powyp2 = (powyp2 * x) % largeP2;
      }

      for (m = 3; m <= maxPow; m++) {
        for (n = 3; n <= maxPow; n++) {
          auto xm1 = pxp1[m - 3];
          auto yn1 = pyp1[n - 3];
          auto hp1Key = (xm1 + yn1) % largeP1;

          if (hp1.hasKey(hp1Key)) {
            auto xm2 = pxp2[m - 3];
            auto yn2 = pyp2[n - 3];
            auto hp2Key = (xm2 + yn2) % largeP2;

            if (hp2.hasKey(hp1Key)) {
              checkValues(x, m, y, n, hp1.getValues(hp1Key), hp2.getValues(hp2Key));
            }
          }
        }
      }

      delete[] pxp1;
      delete[] pxp2;
      delete[] pyp1;
      delete[] pyp2;

      if (!(y % 1000)) {
        oLogFile << "y=" << y << std::endl << std::flush;
      }
    }

    if (!(x % 10)) {
      printf("Completed x=%d\n", x);
      oLogFile << "x=" << x << std::endl << std::flush;

      stopTimerAndPrint();
      startTimer();
    }
  }
}

std::tuple<UlhashHashtable, UlhashHashtable> genZs(uint64 from, uint64 to, uint64 maxBase, uint64 maxPow)
{
  if (verbose) printf("Generating all combinations of z^r...\n");
  oLogFile << "Generating all combinations of z^r..." << std::endl;
  oLogFile << "from z = " << from << std::endl;
  oLogFile << "to z = " << to << std::endl;
  std::cout << "Generating z from " << from << " to " << to << std::endl;
  logCurrentTime();

  uint64 hashsetSize = (to - from + 1)*(maxPow - 3 + 1);

  UlhashHashtable hashtable1 = UlhashHashtable(hashsetSize);
  UlhashHashtable hashtable2 = UlhashHashtable(hashsetSize);

  for (uint64 z = from; z <= to; z++) {
    uint64 n1 = z*z;
    uint64 n2 = n1 % largeP2;
    n1 = n1 % largeP1;

    for (uint64 r = 2; r <= maxPow; r++) {
      hashtable1.addValue(n1, std::make_tuple(z, r));
      hashtable2.addValue(n2, std::make_tuple(z, r));

      n1 = (n1*z) % largeP1;
      n2 = (n2*z) % largeP2;
    }
  }

  hashtable1.optimize();
  hashtable2.optimize();
  return std::make_tuple(hashtable1, hashtable2);
}

uint64 gcd(uint64 u, uint64 v) {
  uint64 k = 0;
  if (u == 0)
    return v;
  if (v == 0)
    return u;
  while ((u & 1) == 0 && (v & 1) == 0) { /* while both u and v are even */
    u >>= 1;   /* shift u right, dividing it by 2 */
    v >>= 1;   /* shift v right, dividing it by 2 */
    k++;       /* add a power of 2 to the final result */
  }
  /* At this point either u or v (or both) is odd */
  do {
    if ((u & 1) == 0)      /* if u is even */
      u >>= 1;           /* divide u by 2 */
    else if ((v & 1) == 0) /* else if v is even */
      v >>= 1;           /* divide v by 2 */
    else if (u >= v)       /* u and v are both odd */
      u = (u - v) >> 1;
    else                   /* u and v both odd, v > u */
      v = (v - u) >> 1;
  } while (u > 0);
  return v << k;  /* returns v * 2^k */
}

void startTimer()
{
  startClock = clock();
}

void stopTimerAndPrint()
{
  clock_t endClock = clock();
  printf("Elapsed: %f seconds\n", (double)(endClock - startClock) / CLOCKS_PER_SEC);
}

void tryGetValueLine(std::string varName, std::string & valLine, std::string line)
{
  if (line.compare(0, 2, varName) == 0)
  {
    valLine = line.substr(2);
  }
}

int parseVal(std::string line, int defaultValue)
{
  if (line.length() > 0)
  {
    return atoi(line.c_str());
  }

  return defaultValue;
}

std::tuple<uint64, uint64> readLogFile(int defaultZ, int defaultX)
{
  std::ifstream iLogFile(logFileName);
  std::string line;
  std::string zString;
  std::string xString;

  // read log
  while (std::getline(iLogFile, line))
  {
    tryGetValueLine("z=", zString, line);
    tryGetValueLine("x=", xString, line);
  }

  int savedZ;
  int savedX;

  savedZ = parseVal(zString, 2);
  savedX = parseVal(xString, 2);

  iLogFile.close();

  return std::make_tuple(savedZ, savedX);
}

const std::string currentDateTime() {
  time_t     now = time(0);
  struct tm  tstruct;
  char       buf[80];
  tstruct = *localtime(&now);
  // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
  // for more information about date/time format
  strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

  return buf;
}

void logCurrentTime()
{
  oLogFile << "now is " << currentDateTime() << std::endl << std::flush;
}