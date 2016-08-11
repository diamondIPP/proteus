#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include <string>

#include "utils/definitions.h"

namespace Mechanics {

  class Device;

  /** A geometric object with defined local coordinates. */
  class Alignable {
  public:
      Alignable(const Transform3& localToGlobal_);
      Alignable(double offsetX, double offsetY, double offsetZ,
                double rotationX, double rotationY, double rotationZ);

      const Transform3& localToGlobal() const { return _l2g; }
      const Transform3& globalToLocal() const { return _g2l; }

  private:
      Transform3 _l2g, _g2l;
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
