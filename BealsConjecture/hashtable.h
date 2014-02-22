#include <map>
#include <tuple>

typedef unsigned __int64 uint64;

class Hashtable
{
public:
  Hashtable()
  {
  }

  Hashtable(int size)
  {
  }
  
  virtual void addValue(uint64 key, std::tuple<uint64, uint64> val)
  {
    hashtable[key] = val;
  }

  virtual bool tryGetValue(uint64 key, std::tuple<uint64, uint64> & val)
  {
    auto iterator = hashtable.find(key);

    if (iterator == hashtable.end())
    {
      val = std::make_tuple(0, 0);
      return false;
    }

    val = iterator->second;
    return true;
  }

  virtual void optimize()
  {
  }

private:
  std::map<uint64, std::tuple<uint64, uint64>> hashtable;
};