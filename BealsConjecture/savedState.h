typedef unsigned __int64 uint64;

struct SavedState
{
  SavedState(uint64 _z, uint64 _x)
  {
    z = _z;
    x = _x;
  }

  uint64 z;
  uint64 x;
};