#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include <string>

#include "utils/definitions.h"

namespace Mechanics {

  class Device;

  class Alignable {
  public:
      Alignable(const Transform3& l2g);
      Alignable(double offsetX, double offsetY, double offsetZ,
                double rotationX, double rotationY, double rotationZ);

  private:
      Transform3 _local2global;
  };

  class Alignment {
  public:
    Alignment(const char* fileName, Device* device);

    const char* getFileName();
    
    void readFile();
    void writeFile();
    
  private:
    unsigned int sensorFromHeader(std::string header);
    
  private:
    std::string _fileName;
    Device* _device;
    
  }; // end of class
  
} // end of namespace

#endif // ALIGNMENT_H
