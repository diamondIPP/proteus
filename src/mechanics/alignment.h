#ifndef ALIGNMENT_H
#define ALIGNMENT_H

#include <map>
#include <string>

#include "utils/definitions.h"

class ConfigParser;

namespace Mechanics {

class Alignment {
public:
  Alignment();
  /** Construct alignment from a configuration file. */
  static Alignment fromFile(const std::string& path);

  void writeFile(const std::string& path) const;
  /** \deprecated Alignment object should know nothing about file paths. */
  void writeFile() const { writeFile(m_path); }

  /** Check if alignment information exsists for the given sensor. */
  bool hasAlignment(Index sensorId) const;
  Transform3D getLocalToGlobal(Index sensorId) const;
  void setOffset(Index sensorId, const XYZPoint& offset);
  void setOffset(Index sensorId, double x, double y, double z);
  void setRotationAngles(Index sensorId, double rotX, double rotY, double rotZ);

  /** Beam direction in the global coordinate system. */
  XYZVector beamDirection() const;
  void setBeamSlope(double slopeX, double slopeY);

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
  std::string m_path;

  // only temporarily until Sensor is using Transform3D;
  friend class Device;
};

} // namespace Mechanics

#endif // ALIGNMENT_H
