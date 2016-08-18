#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include <map>
#include <string>

#include "utils/definitions.h"

class ConfigParser;

namespace Mechanics {

class Device;

class Alignment {
public:
  Alignment(const std::string& fileName);

  const std::string& getFileName() const { return m_fileName; }

  void readFile(const std::string& path);
  void writeFile(const std::string& path) const;
  void writeFile() const { writeFile(m_fileName); }

  /** Check if alignment information exsists for the given sensor. */
  bool hasAlignment(Index sensorId) const;
  Transform3 getLocalToGlobal(Index sensorId) const;
  void setOffset(Index sensorId, const Point3& offset);
  void setRotationAngles(Index sensorId, double rotX, double rotY, double rotZ);

  /** Beam direction in the global coordinate system. */
  Vector3 beamDirection() const;

private:
  struct GeoParams {
    double offsetX, offsetY, offsetZ;
    double rotationX, rotationY, rotationZ;
    // ensure parameters have sensible defaults
    GeoParams()
        : offsetX(0)
        , offsetY(0)
        , offsetZ(0)
        , rotationX(0)
        , rotationY(0)
        , rotationZ(0)
    {
    }
  };

  void parse(const ConfigParser& config);

  std::map<Index, GeoParams> m_geo;
  double m_beamSlopeX, m_beamSlopeY, m_syncRatio;
  std::string m_fileName;
  // only temporarily until Sensor is using Transform3;
  friend class Device;
};

} // namespace Mechanics

#endif // ALIGNMENT_H
