#ifndef PT_GEOMETRY_H
#define PT_GEOMETRY_H

#include <iosfwd>
#include <map>
#include <string>

#include "utils/config.h"
#include "utils/definitions.h"

class ConfigParser;

namespace Mechanics {

/** Store and process geometry parameters for the whole setup.
 *
 * The geometry of each sensor plane is defined by the position
 * [x, y, z] of its origin in the global system and by three rotation
 * angles [alpha, beta, gamma] that define the rotation from the local
 * to the global system using the 3-2-1 convention. The rotation
 * matrix is constructed by rotations around each axis as
 *
 *    R = R_1(alpha) * R_2(beta) * R_3(gamma),
 *
 * i.e. first a rotation by gamma around the local third axis
 * (w coordinate), then a rotation by beta around the updated second
 * axis, followed with a roation by alpha around the first axis.
 *
 * The class also stores uncertainties for the geometry parameters.
 * They are only used transiently and are not stored in the geometry
 * file.
 */
class Geometry {
public:
  Geometry();

  /** Construct alignment from a configuration file. */
  static Geometry fromFile(const std::string& path);
  /** Write alignment to a configuration file. */
  void writeFile(const std::string& path) const;

  /** Construct alignment from old configuration parser. */
  static Geometry fromConfig(const ConfigParser& cfg);
  /** Construct alignment from a configuration object. */
  static Geometry fromConfig(const toml::Value& cfg);
  /** Convert alignment into a configuration object. */
  toml::Value toConfig() const;

  void setOffset(Index sensorId, const XYZPoint& offset);
  void setOffset(Index sensorId, double x, double y, double z);
  void setRotationAngles(Index sensorId, double rotX, double rotY, double rotZ);
  /** Change the global offset by small values. */
  void correctGlobalOffset(Index sensorId, double dx, double dy, double dz);
  /** Change the rotation angles by small values. */
  void correctRotationAngles(Index sensorId,
                             double dalpha,
                             double dbeta,
                             double dgamma);
  /** Add small local corrections du, dv, dw, dRotU, dRotV, dRotW. */
  void
  correctLocal(Index sensorId, const Vector6& delta, const SymMatrix6& cov);
  /** Transformation from local to global coordinates for the sensor. */
  Transform3D getLocalToGlobal(Index sensorId) const;
  /** Geometry parameters x, y, z, alpha, beta, gamma for the sensor. */
  Vector6 getParams(Index sensorId) const;
  /** Geometry parameters covariance matrix. */
  const SymMatrix6& getParamsCov(Index sensorId) const;

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
    SymMatrix6 cov;

    PlaneParams();
  };

  std::map<Index, PlaneParams> m_params;
  double m_beamSlopeX, m_beamSlopeY, m_syncRatio;
};

} // namespace Mechanics

#endif // PT_GEOMETRY_H
