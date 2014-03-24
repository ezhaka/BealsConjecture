#include <fstream>
#include <string>
#include <string.h>
#include <time.h>

class Logger
{
public:
  Logger()
  {
    // todo: make file name const variable
  }

  void logCurrentTime() const
  {
    std::ofstream logFile;
    logFile.open("logfile.txt", std::ios::app);
    logFile << "now is " << currentDateTime() << std::endl << std::flush;
    logFile.close();
  }

private:
  std::string currentDateTime() const
  {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
  }
};