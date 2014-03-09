typedef unsigned __int64 uint64;

struct SavedState
{
  SavedState(uint64 _z, uint64 _x, uint64 _x_from, uint64 _x_to)
  {
    z = _z;
    x = _x;
    x_from = _x_from;
    x_to = _x_to;
  }

  uint64 z;
  uint64 x;
  uint64 x_from;
  uint64 x_to;
};