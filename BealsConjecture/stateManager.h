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
    std::string fromXString;
    std::string toXString;

    while (std::getline(logfile, line))
    {
      tryGetValueLine("z=", zString, line);
      tryGetValueLine("x=", xString, line);
      tryGetValueLine("x_from=", fromXString, line);
      tryGetValueLine("x_to=", toXString, line);
    }

    logfile.close();

    SavedState savedState(defaultState.z, defaultState.x, defaultState.x_from, defaultState.x_to);

    savedState.z = parseVal(zString, defaultState.z);
    savedState.x = parseVal(xString, defaultState.x);
    savedState.x_to = parseVal(toXString, defaultState.x_to);
    savedState.x_from = parseVal(fromXString, defaultState.x_from);

    return savedState;
  }

private:
  void tryGetValueLine(std::string varName, std::string & valLine, std::string line) const
  {
    if (line.compare(0, varName.size(), varName) == 0)
    {
      valLine = line.substr(varName.size());
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