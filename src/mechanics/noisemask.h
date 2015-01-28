#ifndef NOISEMASK_H
#define NOISEMASK_H

#include <vector>
#include <fstream>
#include <sstream>
#include <string>

namespace Loopers { class NoiseScanConfig; }

namespace Mechanics {

  class Device;

  /** @class NoiseMask
   **    
   ** @brief A class to deal with masked pixels.
   **
   ** Writes and reads from a textfile of all noisy pixels 
   ** in the format:
   **      sensor, x, y
   */
  class NoiseMask {
    
  public:
    NoiseMask(const char* fileName, Device* device);
    ~NoiseMask();

    void writeMask(const Loopers::NoiseScanConfig* config);
    void readMask();
    
    std::vector<bool**> getMaskArrays() const;

    const char* getFileName() { return _fileName.c_str(); }
    const Device* getDevice() const { return _device; }
    const Loopers::NoiseScanConfig* getConfig() const { return _config; }
    
  private:
    void parseLine(std::stringstream& line,
		   unsigned int& nsens,
		   unsigned int& x,
		   unsigned int& y);

    void parseComments(std::stringstream& comments);
    
  private:
    std::string _fileName;
    Device* _device;
    Loopers::NoiseScanConfig* _config;

  }; // end of class
  
} // end of namespace

#endif // NOISEMASK_H
