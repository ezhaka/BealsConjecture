#include "hashtable.h"
#include "ulhash3.h"
#include <map>
#include <tuple>
#include <vector>

class UlhashHashtable
{
public:
  UlhashHashtable()
  {
  }

  UlhashHashtable(int size)
  {
    hashtable = ulhash_create(size);
  }
  
  void addValue(uint64 key, std::tuple<uint64, uint64> val)
  {
    ulhash_set(hashtable, key);
  }

  bool hasKey(uint64 key)
  {
    bool result = ulhash_opt_find(hashtable, key); 

    return result;
  }

  std::vector<std::tuple<uint64, uint64>> getValues(uint64 key)
  {
    return stdMap[key];
  }

  void optimize()
  {
    ulhash_opt(hashtable);
  }

  void free()
  {
    ulhash_free(hashtable);
    stdMap.clear();
  }

private:
  ulhash* hashtable;
  std::map<uint64, std::vector<std::tuple<uint64, uint64>>> stdMap;
};