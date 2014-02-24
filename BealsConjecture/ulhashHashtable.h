#include "hashtable.h"
#include "ulhash3.h"

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

  bool tryGetValue(uint64 key)
  {
    bool result = ulhash_opt_find(hashtable, key); 
    //val = std::make_tuple(0, 0);

    return result;
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