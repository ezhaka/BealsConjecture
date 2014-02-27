#include "ulhash3.h"
#include <map>
#include <tuple>
#include <vector>
#include <iostream>
#include "constants.h"

class UlhashHashtable
{
public:
  UlhashHashtable(int size)
  {
    hashtable = ulhash_create(size);
  }

  ~UlhashHashtable()
  {
  }
  
  void addValue(uint64 key, std::tuple<uint64, uint64> val)
  {
    ulhash_set(hashtable, key);
  }

  bool hasKey(uint64 key)
  {
    return ulhash_opt_find(hashtable, key); 
  }

  void optimize()
  {
    ulhash_opt(hashtable);
  }

  void free()
  {
    ulhash_free(hashtable);
  }

private:
  ulhash* hashtable;
};