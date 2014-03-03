#include "savedState.h"
#include <fstream>
#include <string.h>
#include <string>

class StateManager
{
public:
  SavedState load(SavedState defaultState) const
  {
    std::ifstream logfile("logfile.txt");
    std::string line;
    std::string zString;
    std::string xString;

    while (std::getline(logfile, line))
    {
      tryGetValueLine("z=", zString, line);
      tryGetValueLine("x=", xString, line);
    }

    logfile.close();

    SavedState savedState(defaultState.z, defaultState.x);

    savedState.z = parseVal(zString, defaultState.z);
    savedState.x = parseVal(xString, defaultState.x);

    return savedState;
  }

private:
  void tryGetValueLine(std::string varName, std::string & valLine, std::string line) const
  {
    if (line.compare(0, 2, varName) == 0)
    {
      valLine = line.substr(2);
    }
  }

  int parseVal(std::string line, int defaultValue) const
  {
    if (line.length() > 0)
    {
      return atoi(line.c_str());
    }

    return defaultValue;
  }
};