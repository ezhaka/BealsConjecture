#include "constants.h"
#include <iostream>

class ModuloHelper
{
public:
  uint64 pow(uint64 base, uint64 pow, uint64 modulo)
  {
    uint64 result = (base * base) % modulo;

    for (uint64 r = 2; r < pow; r++) 
    {
      result = (result * base) % modulo;
      std::cout << result << " " << r + 1 << std::endl;
    }

    return result;
  }
};