#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include <map>
#include <string>

#include "utils/definitions.h"

class ConfigParser;

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

    const char* getFileName() { return _fileName.c_str(); }
    
    void readFile(const std::string& path);
    void writeFile(const std::string& path);
    void readFile() { readFile(_fileName); }
    void writeFile() { writeFile(_fileName); }

    bool hasAlignment(Index sensor_id) const;
    Transform3 getLocalToGlobal(Index sensor_id) const;

  private:
    struct Geometry {
      double offsetX, offsetY, offsetZ;
      double rotationX, rotationY, rotationZ;

      Geometry() : offsetX(0), offsetY(0), offsetZ(0), rotationX(0), rotationY(0), rotationZ(0) {}
    };
    typedef std::map<Index,Geometry> Geometries;

    void parse(const ConfigParser& config);

    Geometries _geo;
    double _beamSlopeX, _beamSlopeY, _syncRatio;
    std::string _fileName;
    Device* _device;
    
  }; // end of class
  
} // end of namespace

#endif // ALIGNMENT_H
