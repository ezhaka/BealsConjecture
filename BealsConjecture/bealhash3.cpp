#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ulhash3.h"
#include <cilk\cilk.h>
#include <time.h>
#include "ulhashHashtable.h"

uint64 minPow  = 0;
uint64 maxPow  = 0;
uint64 minBase = 0;
uint64 maxBase = 0;
unsigned int numCand = 0;

//ulhash* hp1;
//ulhash* hp2;

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
void genZs();
void precomputePows();
void checkSums();
int checkModPrime(int x, int m, int y, int n);
void addValue(uint64 key, std::tuple<uint64, uint64> val);
bool tryGetValue(uint64 key, std::tuple<uint64, uint64> & val);
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
  
  minBase = atol(argv[1]);
  maxBase = atol(argv[2]);
  minPow = atol(argv[3]);
  maxPow = atol(argv[4]);

  

  // Generate the possible z^r values and put them in the trie
  if (verbose) printf("Generating all combinations of z^r...\n");
  genZs();

  if (verbose) {
  	//printf("mod largeP1:\n");
  	//ulhash_print_stats(hp1);
  	//printf("mod largeP2:\n");
  	//ulhash_print_stats(hp2);
  
  	printf("Done. Precomputing powers...\n");
  }
  precomputePows();

  if (verbose) printf("Done. Searching for candidates...\n");
  
  // Check each sum for inclusion in the table modulo 2^(word size)
  checkSums();
  
  printf("Finished. A total of %d candidates were found.\n", numCand);

  getchar();
}

// Check each x^m + y^n w/ gcd(m,n)==1 for inclusion in the trie
// Output each as a candidate if it matches.
void checkSums() {
	unsigned int x, y, m, n;
	uint64 *powx1, *powy1;
	uint64 *powx2, *powy2;
	uint64 xm1, xm2;

  startTimer();
	
	for (x=minBase; x<=maxBase; x++) {
		powx1 = powsp1[x-minBase];
		powx2 = powsp2[x-minBase];
		
		for (y=minBase; y<=x; y++) {
			if (useGcd && gcd(x,y) != 1) continue;
			powy1 = powsp1[y-minBase];
			powy2 = powsp2[y-minBase];
			
			for (m=minPow; m<=maxPow; m++) {
        //if (!(m%10)) fprintf(stderr,"Completed m=%d\n", m);
				xm1 = powx1[m-minPow];
				xm2 = powx2[m-minPow];
				
				for (n=minPow; n<=maxPow; n++) {
        //if (!(n%10)) fprintf(stderr,"Completed n=%d\n", n);
          auto yn1 = powy1[n-minPow];
          auto yn2 = powy2[n-minPow];

          std::tuple<uint64, uint64> hp1res;
          std::tuple<uint64, uint64> hp2res;

          if (hp1.tryGetValue((xm1 + yn1)%largeP1, hp1res)) {
						if (hp2.tryGetValue((xm2 + yn2)%largeP2, hp2res)) {
              if (std::get<0>(hp1res) == std::get<0>(hp2res)) {
							  numCand++;
                printf("%u^%u + %u^%u = %u^%u\n", x, m, y, n, std::get<0>(hp1res), std::get<1>(hp1res));
              }
            }
					}
				}
			}

      //if (!(y%10)) fprintf(stderr,"Completed y=%d\n", x);
		}
		
		if (!(x%10)) {
      printf("Completed x=%d\n", x);
      fprintf(stderr,"Completed x=%d\n", x);

      stopTimerAndPrint();
      startTimer();
    }
	}
}

// Initialize the two tries, and generate z^r for each z&r in range
// t2 is calculated using machine words, and tp is modulo a largeish prime
void genZs() {
  int z, r;
  uint64 n1, n2;
  
  // We'll be inserting ~maxBase*maxPow entries into the hash tables
  uint64 hashsetSize = (maxBase-minBase+1)*(maxPow-minPow+1);
  //hp1 = ulhash_create(hashsetSize);
  //hp2 = ulhash_create(hashsetSize);
  hp1 = UlhashHashtable(hashsetSize);
  hp2 = UlhashHashtable(hashsetSize);
  
  for (z=minBase; z<=maxBase; z++) {
    n1 = z*z;
    n2 = n1 % largeP2;
    n1 = n1 % largeP1;
    
    for (r=2; r<=maxPow; r++) {
      if (r >= minPow) {
        hp1.addValue(n1, std::make_tuple(z, r));
        hp2.addValue(n2, std::make_tuple(z, r));

			  //ulhash_set(hp1, n1);
        //ulhash_set(hp2, n2);
      }

      n1 = (n1*z) % largeP1;
      n2 = (n2*z) % largeP2;
    }
  }
  
  hp1.optimize();
  hp2.optimize();
  //ulhash_opt(hp1);
  //ulhash_opt(hp2);
}

// Precompute powers modulo 2^{word size}
void precomputePows() {
  uint64 powspSize = (maxBase-minBase+1) * sizeof(uint64*);
	powsp1 = new uint64* [maxBase-minBase+1];// (uint64**) malloc( powspSize );
	powsp2 = new uint64* [maxBase-minBase+1];
	
	for (int x=minBase; x<=maxBase; x++) {
    uint64 pxpSize = (maxPow-minPow+1) * sizeof(uint64);
		uint64* pxp1 = new uint64 [maxPow-minPow+1]; // (uint64*) malloc( pxpSize );
		uint64* pxp2 = new uint64 [maxPow-minPow+1]; // (uint64*) malloc( pxpSize );
		uint64 powxp1 = (x*x) % largeP1;
		uint64 powxp2 = (x*x) % largeP2;
		powsp1[x-minBase] = pxp1;
		powsp2[x-minBase] = pxp2;
		
		for (int m=2; m<=maxPow; m++) {
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