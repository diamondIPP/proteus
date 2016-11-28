#ifndef PT_ALIGNMENT_H
#define PT_ALIGNMENT_H

#include <iosfwd>
#include <map>
#include <string>

#include "utils/config.h"
#include "utils/definitions.h"

class ConfigParser;

namespace Mechanics {

/** Store and process alignment parameters. */
class Alignment {
public:
  Alignment();

  /** Construct alignment from a configuration file. */
  static Alignment fromFile(const std::string& path);
  /** Write alignment to a configuration file. */
  void writeFile(const std::string& path) const;

  /** Construct alignment from old configuration parser. */
  static Alignment fromConfig(const ConfigParser& cfg);
  /** Construct alignment from a configuration object. */
  static Alignment fromConfig(const toml::Value& cfg);
  /** Convert alignment into a configuration object. */
  toml::Value toConfig() const;

  void setOffset(Index sensorId, const XYZPoint& offset);
  void setOffset(Index sensorId, double x, double y, double z);
  void setRotationAngles(Index sensorId, double rotX, double rotY, double rotZ);
  /** Change the global offset by small values. */
  void correctGlobalOffset(Index sensorId, double dx, double dy, double dz);
  /** Change the rotation by small values around the current rotation angles. */
  void correctRotationAngles(Index sensorId,
                             double dalpha,
                             double dbeta,
                             double dgamma);
  /** Add small local corrections du, dv, dw, rotU, rotV, rotW. */
  void correctLocal(Index sensorId, const Vector6& delta);
  /** Check if alignment information exsists for the given sensor. */
  bool hasAlignment(Index sensorId) const;
  /** Transformation from local to global coordinates for the sensor. */
  Transform3D getLocalToGlobal(Index sensorId) const;
  /** Geometry parameters x, y, z, alpha, beta, gamma for the sensor */
  Vector6 getParams(Index sensorId) const;

  void setBeamSlope(double slopeX, double slopeY);
  /** Beam direction in the global coordinate system. */
  XYZVector beamDirection() const;

  void setSyncRatio(double ratio) { m_syncRatio = ratio; }
  double syncRatio() const { return m_syncRatio; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  struct PlaneParams {
    double offsetX, offsetY, offsetZ;
    double rotationX, rotationY, rotationZ;

    PlaneParams();
  };

  std::map<Index, PlaneParams> m_params;
  double m_beamSlopeX, m_beamSlopeY, m_syncRatio;
};

} // namespace Mechanics

#endif // PT_ALIGNMENT_H
