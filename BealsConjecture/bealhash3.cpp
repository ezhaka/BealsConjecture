#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include "ulhash3.h"
#include <cilk\cilk.h>
#include <time.h>
#include "ulhashHashtable.h"
#include <cstdio>

uint64 maxPow  = 0;
uint64 maxBase = 0;

uint64 savedX = 0;
uint64 savedZ = 0;

unsigned int numCand = 0;

std::ofstream oLogFile;
std::string logFileName = "logfile.txt";
void readLogFile();

UlhashHashtable hp1;
UlhashHashtable hp2;

uint64** powsp1;
uint64** powsp2;

uint64 largeP1 = 4294967291ULL; //(1<<31) - 5;
uint64 largeP2 = 4294967279ULL; //(1<<31) - 17;

clock_t startClock;
void startTimer();
void stopTimerAndPrint();

uint64 gcd(uint64 u, uint64 v);
void genZs(uint64 from, uint64 to);
void precomputePows();
void checkSums(int fromX, int toX);
int checkModPrime(int x, int m, int y, int n);
void addValue(uint64 key, std::tuple<uint64, uint64> val);
bool tryGetValue(uint64 key, std::tuple<uint64, uint64> & val);
void logCurrentTime();
int verbose = 0;
int useGcd = 1;

// Use two tries -- one is the Z^r modulo 2^(word size)
// the other is modulo a large prime, ~2^(half word size)
// We screen through both to get good accuracy w/o the 

int main(int argc, char** argv) {
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

  maxBase = atol(argv[3]);
  maxPow = atol(argv[4]);

  savedX = 2;
  savedZ = 2;

  readLogFile();
  oLogFile.open(logFileName, std::ios::app);

  int zStep = 10000;
  int fromZ = savedZ == 2 ? 1 : ((savedZ / zStep) + 1);

  for (uint64 z = fromZ; z <= maxBase / zStep; z++)
  {
    genZs((z - 1) * zStep, z * zStep);

    if (verbose) printf("Done. Searching for candidates...\n");
  
    // Check each sum for inclusion in the table modulo 2^(word size)
    checkSums(z == fromZ ? savedX : fromX, toX);
  
    printf("Finished. A total of %d candidates were found.\n", numCand);

    hp1.free();
    hp2.free();
    oLogFile << "z=" << z * zStep << std::endl << std::flush;
  }
  //getchar();
}

uint64 modularPow(uint64 base, uint64 pow, uint64 modulo)
{
	uint64 result = (base*base) % modulo;
		
	for (int m=2; m<=pow; m++) {
    result = (result * base) % modulo;
	}

  return result;
}

// Check each x^m + y^n w/ gcd(m,n)==1 for inclusion in the trie
// Output each as a candidate if it matches.
void checkSums(int fromX, int toX) {
	unsigned int x, y, m, n;

  startTimer();
	
 	for (x=fromX; x<=toX; x++) {
		for (y = 2; y<=x; y++) {
			if (useGcd && gcd(x,y) != 1) continue;

      // compute modular powers here
		  uint64* pxp1 = new uint64 [maxPow-3+1];
		  uint64* pxp2 = new uint64 [maxPow-3+1];
      uint64* pyp1 = new uint64 [maxPow-3+1];
		  uint64* pyp2 = new uint64 [maxPow-3+1];
		  uint64 powxp1 = (x*x) % largeP1;
		  uint64 powxp2 = (x*x) % largeP2;
      uint64 powyp1 = (y*y) % largeP1;
		  uint64 powyp2 = (y*y) % largeP2;
		
		  for (int m=2; m<=maxPow; m++) {
        int minPow = 3;

        if (m >= minPow) {
			    pxp1[m-minPow] = powxp1;
			    pxp2[m-minPow] = powxp2;
          pyp1[m-minPow] = powyp1;
			    pyp2[m-minPow] = powyp2;
        }

        powxp1 = (powxp1 * x) % largeP1;
			  powxp2 = (powxp2 * x) % largeP2;
        powyp1 = (powyp1 * x) % largeP1;
			  powyp2 = (powyp2 * x) % largeP2;
		  }

			for (m = 3; m<=maxPow; m++) {
				for (n = 3; n<=maxPow; n++) {
          auto xm1 = pxp1[m-3];
          auto yn1 = pyp1[n-3];

          if (hp1.tryGetValue((xm1 + yn1)%largeP1)) {
            auto xm2 = pxp2[m-3];
            auto yn2 = pyp2[n-3];

						if (hp2.tryGetValue((xm2 + yn2)%largeP2)) {
							numCand++;
              printf("%u^%u + %u^%u\n", x, m, y, n);
              oLogFile << x << "^" << m << " + " << y << "^" << n << std::endl;
            }
					}
				}
			}

      delete[] pxp1;
      delete[] pxp2;
      delete[] pyp1;
      delete[] pyp2;

      if (!(y%100)) {
        oLogFile << "y=" << y << std::endl << std::flush;
      }
		}
		
		if (!(x%10)) {
      printf("Completed x=%d\n", x);
      oLogFile << "x=" << x << std::endl << std::flush;

      stopTimerAndPrint();
      startTimer();
    }
	}
}

// Initialize the two tries, and generate z^r for each z&r in range
// t2 is calculated using machine words, and tp is modulo a largeish prime
void genZs(uint64 from, uint64 to) {

  if (verbose) printf("Generating all combinations of z^r...\n");
  oLogFile << "Generating all combinations of z^r..." << std::endl;
  oLogFile << "from z = " << from << std::endl;
  oLogFile << "to z = " << to << std::endl;
  std::cout << "Generating z from " << from << " to " << to << std::endl;
  logCurrentTime();

  int z, r;
  uint64 n1, n2;
  
  // We'll be inserting ~maxBase*maxPow entries into the hash tables
  uint64 hashsetSize = (to-from+1)*(maxPow-3+1);
  hp1 = UlhashHashtable(hashsetSize);
  hp2 = UlhashHashtable(hashsetSize);
  
  for (z=from; z<=to; z++) {
    n1 = z*z;
    n2 = n1 % largeP2;
    n1 = n1 % largeP1;
    
    for (r=2; r<=maxPow; r++) {
      hp1.addValue(n1, std::make_tuple(z, r));
      hp2.addValue(n2, std::make_tuple(z, r));

      n1 = (n1*z) % largeP1;
      n2 = (n2*z) % largeP2;
    }
  }
  
  hp1.optimize();
  hp2.optimize();
}

// Precompute powers modulo 2^{word size}
void precomputePows() {
  uint64 powspSize = (maxBase-2+1) * sizeof(uint64*);
	powsp1 = new uint64* [maxBase-2+1];// (uint64**) malloc( powspSize );
	powsp2 = new uint64* [maxBase-2+1];
	
	for (int x=2; x<=maxBase; x++) {
    uint64 pxpSize = (maxPow-3+1) * sizeof(uint64);
		uint64* pxp1 = new uint64 [maxPow-3+1]; // (uint64*) malloc( pxpSize );
		uint64* pxp2 = new uint64 [maxPow-3+1]; // (uint64*) malloc( pxpSize );
		uint64 powxp1 = (x*x) % largeP1;
		uint64 powxp2 = (x*x) % largeP2;
		powsp1[x-2] = pxp1;
		powsp2[x-2] = pxp2;
		
		for (int m=2; m<=maxPow; m++) {
      int minPow = 3;

      if (m >= minPow) {
			  pxp1[m-minPow] = powxp1;
			  pxp2[m-minPow] = powxp2;
      }

      powxp1 = (powxp1 * x) % largeP1;
			powxp2 = (powxp2 * x) % largeP2;
		}
	}
}


uint64 gcd(uint64 u, uint64 v) {
     uint64 k = 0;
     if (u == 0)
         return v;
     if (v == 0)
         return u;
     while ((u & 1) == 0  &&  (v & 1) == 0) { /* while both u and v are even */
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
             u = (u-v) >> 1;
         else                   /* u and v both odd, v > u */
             v = (v-u) >> 1;
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

void parseVal(std::string line, uint64 & val)
{
  if (line.length() > 0)
  {
    val = atoi(line.c_str());
  }
}

void readLogFile()
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

  parseVal(zString, savedZ);
  parseVal(xString, savedX);
  
  iLogFile.close();
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