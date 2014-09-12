#ifndef NOISEMASK_H
#define NOISEMASK_H

#include <vector>
#include <fstream>
#include <sstream>

/*******************************************************************************
 * Writes and reads from a textfile of all noisy pixels in the format:
 * sensor, x, y
 */

namespace Mechanics {

class Device;

class NoiseMask
{
private:
  const char* _fileName;
  Device* _device;

  void parseLine(std::stringstream& line, unsigned int& nsens,
                 unsigned int& x, unsigned int& y);

public:
  NoiseMask(const char* fileName, Device* device);

  void writeMask();
  void readMask();

  std::vector<bool**> getMaskArrays() const;

  const char* getFileName();
};

}

#endif // NOISEMASK_H
