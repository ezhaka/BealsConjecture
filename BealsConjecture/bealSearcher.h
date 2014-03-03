#include <tuple>
#include <fstream>
#include <string>
#include <string.h>
#include "ulhashHashtable.h"
#include "logger.h"
#include <cilk\cilk.h>
#include <tbb\mutex.h>

class BealSearcher
{
public:
  BealSearcher()
  {
    logStream.open("logfile.txt", std::ios::app);
  }

  std::tuple<UlhashHashtable, UlhashHashtable> genZs(uint64 from, uint64 to, uint64 maxBase, uint64 maxPow)
  {
    logStream << "from z = " << from << std::endl;
    logStream << "to z = " << to << std::endl;
    logger.logCurrentTime();

    uint64 hashsetSize = (to - from + 1)*(maxPow - 3 + 1);

    UlhashHashtable hashtable1 = UlhashHashtable(hashsetSize);
    UlhashHashtable hashtable2 = UlhashHashtable(hashsetSize);

    for (uint64 z = from; z <= to; z++) 
    {
      uint64 n1 = z*z;
      uint64 n2 = n1 % constants::largeP2;
      n1 = n1 % constants::largeP1;

      for (uint64 r = 2; r <= maxPow; r++) 
      {
        if (r > 2)
        {
          hashtable1.addValue(n1, std::make_tuple(z, r));
          hashtable2.addValue(n2, std::make_tuple(z, r));
        }

        n1 = (n1*z) % constants::largeP1;
        n2 = (n2*z) % constants::largeP2;
      }
    }

    hashtable1.optimize();
    hashtable2.optimize();
    return std::make_tuple(hashtable1, hashtable2);
  }

  void checkSums(int fromX, int toX, int maxPow, std::tuple<UlhashHashtable, UlhashHashtable> hashtables) 
  {
    UlhashHashtable hp1 = std::get<0>(hashtables);
    UlhashHashtable hp2 = std::get<1>(hashtables);

    for (int x = fromX; x <= toX; x++) 
    {
      cilk_for (int y = 2; y <= x; y++) 
      {
        if (gcd(x, y) != 1)
        {
          continue;
        }

        uint64* pxp1 = new uint64[maxPow - 3 + 1];
        uint64* pxp2 = new uint64[maxPow - 3 + 1];
        uint64* pyp1 = new uint64[maxPow - 3 + 1];
        uint64* pyp2 = new uint64[maxPow - 3 + 1];
        uint64 powxp1 = (x*x) % constants::largeP1;
        uint64 powxp2 = (x*x) % constants::largeP2;
        uint64 powyp1 = (y*y) % constants::largeP1;
        uint64 powyp2 = (y*y) % constants::largeP2;

        for (int m = 2; m <= maxPow; m++) 
        {
          int minPow = 3;

          if (m >= minPow) 
          {
            pxp1[m - minPow] = powxp1;
            pxp2[m - minPow] = powxp2;
            pyp1[m - minPow] = powyp1;
            pyp2[m - minPow] = powyp2;
          }

          powxp1 = (powxp1 * x) % constants::largeP1;
          powxp2 = (powxp2 * x) % constants::largeP2;
          powyp1 = (powyp1 * y) % constants::largeP1;
          powyp2 = (powyp2 * y) % constants::largeP2;
        }

        for (int m = 3; m <= maxPow; m++) 
        {
          for (int n = 3; n <= maxPow; n++) 
          {
            auto xm1 = pxp1[m - 3];
            auto yn1 = pyp1[n - 3];
            auto hp1Key = (xm1 + yn1) % constants::largeP1;

            if (hp1.hasKey(hp1Key)) 
            {
              auto xm2 = pxp2[m - 3];
              auto yn2 = pyp2[n - 3];
              auto hp2Key = (xm2 + yn2) % constants::largeP2;

              if (hp2.hasKey(hp2Key)) 
              {
                logMutex.lock();
                printf("%d^%d + %d^%d\n", x, m, y, n);
                logStream << x << "^" << m << " + " << y << "^" << n << std::endl;
                logMutex.unlock();
              }
            }
          }
        }

        delete[] pxp1;
        delete[] pxp2;
        delete[] pyp1;
        delete[] pyp2;

        if (!(y % 1000)) 
        {
          logMutex.lock();
          logStream << "y=" << y << std::endl;
          logMutex.unlock();
        }
      }

      if (!(x % 10)) 
      {
        logMutex.lock();
        printf("Completed x=%d\n", x);
        logStream << "x=" << x << std::endl;
        logMutex.unlock();
      }
    }
  }


private:
  std::ofstream logStream;
  Logger logger;
  tbb::mutex logMutex;

  uint64 gcd(uint64 u, uint64 v) const
  {
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
};