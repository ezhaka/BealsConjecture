#include "hashtable.h"
#include "ulhash3.h"
#include <map>
#include <tuple>
#include <vector>
#include <iostream>

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
    auto iter = stdMap.find(key);

    if (iter != stdMap.end())
    {
      iter->second->push_back(val);
    }
    else
    {
      auto pVector = new std::vector<std::tuple<uint64, uint64>>();
      pVector->push_back(val);
      stdMap[key] = pVector;
    }
  }

  bool hasKey(uint64 key)
  {
    return ulhash_opt_find(hashtable, key); 
  }

  std::vector<std::tuple<uint64, uint64>> getValues(uint64 key)
  {
    return *stdMap[key];
  }

  void optimize()
  {
    ulhash_opt(hashtable);
  }

  void free()
  {
    ulhash_free(hashtable);

    for (auto mapItem = stdMap.begin(); mapItem != stdMap.end(); mapItem++)
    {
      std::vector<std::tuple<uint64, uint64>>* pVector = mapItem->second;
      delete pVector;
    }
  }

private:
  ulhash* hashtable;
  std::map<uint64, std::vector<std::tuple<uint64, uint64>>*> stdMap;
};