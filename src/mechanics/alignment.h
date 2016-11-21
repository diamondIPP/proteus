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

  /** Check if alignment information exsists for the given sensor. */
  bool hasAlignment(Index sensorId) const;
  Transform3D getLocalToGlobal(Index sensorId) const;
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
  /** Update using small corrections to the local sensor orientation. */
  void correctLocal(Index sensorId, const Vector3& dq, const Matrix3& dr);

  /** Beam direction in the global coordinate system. */
  XYZVector beamDirection() const;
  void setBeamSlope(double slopeX, double slopeY);

  double syncRatio() const { return m_syncRatio; }
  void setSyncRatio(double ratio) { m_syncRatio = ratio; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

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

  std::map<Index, GeoParams> m_geo;
  double m_beamSlopeX, m_beamSlopeY, m_syncRatio;
};

} // namespace Mechanics

#endif // PT_ALIGNMENT_H
