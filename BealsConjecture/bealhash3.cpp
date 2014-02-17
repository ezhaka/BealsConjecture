#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ulhash3.h"
#include <cilk\cilk.h>
#include <time.h>

unsigned long maxPow  = 0;
unsigned long maxBase = 0;
unsigned int numCand = 0;

ulhash* hp1;
ulhash* hp2;
unsigned long** powsp1;
unsigned long** powsp2;

unsigned long largeP1 = 4294967291ULL; //(1<<31) - 5;
unsigned long largeP2 = 4294967279ULL; //(1<<31) - 17;

unsigned long gcd(unsigned long u, unsigned long v);
void genZs();
void precomputePows();
void checkSums();
int checkModPrime(int x, int m, int y, int n);
int verbose = 0;
int useGcd = 1;

// Use two tries -- one is the Z^r modulo 2^(word size)
// the other is modulo a large prime, ~2^(half word size)
// We screen through both to get good accuracy w/o the 

int main(int argc, char** argv) {
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
  
  maxBase = atol(argv[1]);
  maxPow = atol(argv[2]);

  clock_t startClock = clock();

  // Generate the possible z^r values and put them in the trie
  if (verbose) printf("Generating all combinations of z^r...\n");
  genZs();

  if (verbose) {
  	printf("mod largeP1:\n");
  	ulhash_print_stats(hp1);
  	printf("mod largeP2:\n");
  	ulhash_print_stats(hp2);
  
  	printf("Done. Precomputing powers...\n");
  }
  precomputePows();

  if (verbose) printf("Done. Searching for candidates...\n");
  
  // Check each sum for inclusion in the table modulo 2^(word size)
  checkSums();

  clock_t endClock = clock();
  printf("Elapsed: %f seconds\n", (double)(endClock - startClock) / CLOCKS_PER_SEC);
  
  printf("Finished. A total of %d candidates were found.\n", numCand);

  getchar();
}

// Check each x^m + y^n w/ gcd(m,n)==1 for inclusion in the trie
// Output each as a candidate if it matches.
void checkSums() {
	unsigned int x, y, m, n;
	unsigned long *powx1, *powy1;
	unsigned long *powx2, *powy2;
	unsigned long xm1, xm2;
	
	for (x=2; x<=maxBase; x++) {
		powx1 = powsp1[x-2];
		powx2 = powsp2[x-2];
		
		for (y=2; y<=x; y++) {
			if (useGcd && gcd(x,y) != 1) continue;
			powy1 = powsp1[y-2];
			powy2 = powsp2[y-2];
			
			for (m=2; m<=maxPow; m++) {
				xm1 = powx1[m-2];
				xm2 = powx2[m-2];
				
				for (n=2; n<=maxPow; n++) {
					if (ulhash_opt_find(hp1, (xm1 + powy1[n-2])%largeP1)) {
						if (ulhash_opt_find(hp2, (xm2 + powy2[n-2])%largeP2)) {
							numCand++;
							//printf("%u^%u + %u^%u\n", x, m, y, n);
						}
					}
				}
			}
		}
		
		if (!(x%10)) fprintf(stderr,"Completed x=%d\n", x);
	}
}

// Initialize the two tries, and generate z^r for each z&r in range
// t2 is calculated using machine words, and tp is modulo a largeish prime
void genZs() {
  int z, r;
  unsigned long n1, n2;
  
  // We'll be inserting ~maxBase*maxPow entries into the hash tables
  hp1 = ulhash_create((maxBase-2)*(maxPow-3));
  hp2 = ulhash_create((maxBase-2)*(maxPow-3));
  
  for (z=2; z<=maxBase; z++) {
    n1 = z*z;
    n2 = n1 % largeP2;
    n1 = n1 % largeP1;
    
    for (r=3; r<=maxPow; r++) {
      n1 = (n1*z) % largeP1;
      n2 = (n2*z) % largeP2;
      
      ulhash_set(hp1, n1);
      ulhash_set(hp2, n2);
    }
  }
  
  ulhash_opt(hp1);
  ulhash_opt(hp2);
}

// Precompute powers modulo 2^{word size}
void precomputePows() {
	powsp1 = (unsigned long**) malloc( (maxBase-1) * sizeof(unsigned long*));
	powsp2 = (unsigned long**) malloc( (maxBase-1) * sizeof(unsigned long*));
	
	for (int x=2; x<=maxBase; x++) {
		unsigned long* pxp1 = (unsigned long*) malloc( (maxPow-2) * sizeof(unsigned long));
		unsigned long* pxp2 = (unsigned long*) malloc( (maxPow-2) * sizeof(unsigned long));
		unsigned long powxp1 = (x*x) % largeP1;
		unsigned long powxp2 = (x*x) % largeP2;
		powsp1[x-2] = pxp1;
		powsp2[x-2] = pxp2;
		
		for (int m=3; m<=maxPow; m++) {
			powxp1 = (powxp1 * x) % largeP1;
			powxp2 = (powxp2 * x) % largeP2;
			pxp1[m-3] = powxp1;
			pxp2[m-3] = powxp2;
		}
	}
}

unsigned long gcd(unsigned long u, unsigned long v) {
     unsigned long k = 0;
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
