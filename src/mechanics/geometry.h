#ifndef PT_GEOMETRY_H
#define PT_GEOMETRY_H

#include <iosfwd>
#include <map>
#include <string>

#include "utils/config.h"
#include "utils/definitions.h"

namespace Mechanics {

/** A two-dimensional plane in three-dimensional space.
 *
 * The plane is defined by two internal, orthogonal axes, and one offset
 * that describes the origin of the plane in the global coordinates.
 */
struct Plane {
  Matrix3 rotation; // from local to global coordinates
  Vector3 offset;   // position of the origin in global coordinates

  static Plane
  fromAnglesZYX(double rotZ, double rotY, double rotX, const Vector3& offset);
  static Plane fromDirections(const Vector3& dirU,
                              const Vector3& dirV,
                              const Vector3& offset);

  /** Compute the equivalent Transform3D for the local-to-global transform. */
  Transform3D asTransform3D() const;
  /** Compute minimal parameter vector [x, y, z, rotX, rotY, rotZ]. */
  Vector6 asParams() const;

  Vector3 unitU() const { return rotation.SubCol<Vector3>(0); }
  Vector3 unitV() const { return rotation.SubCol<Vector3>(1); }
  Vector3 unitNormal() const { return rotation.SubCol<Vector3>(2); }
};

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

  /** Construct geometry from a configuration file. */
  static Geometry fromFile(const std::string& path);
  /** Write alignment to a configuration file. */
  void writeFile(const std::string& path) const;

  /** Construct geometry from a configuration object. */
  static Geometry fromConfig(const toml::Value& cfg);
  /** Convert geometry into a configuration object. */
  toml::Value toConfig() const;

  /** Change the global offset by small values. */
  void correctGlobalOffset(Index sensorId, double dx, double dy, double dz);
  /** Add small local corrections du, dv, dw, dRotU, dRotV, dRotW. */
  void
  correctLocal(Index sensorId, const Vector6& delta, const SymMatrix6& cov);
  /** Transformation from local to global coordinates for the sensor. */
  Transform3D getLocalToGlobal(Index sensorId) const;
  /** Geometry parameters x, y, z, alpha, beta, gamma for the sensor. */
  Vector6 getParams(Index sensorId) const;
  /** Geometry parameters covariance matrix. */
  SymMatrix6 getParamsCov(Index sensorId) const;

  void setBeamSlope(double slopeX, double slopeY);
  /** Beam direction in the global coordinate system. */
  XYZVector beamDirection() const;

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  std::map<Index, Plane> m_planes;
  std::map<Index, SymMatrix6> m_covs;
  double m_beamSlopeX, m_beamSlopeY;
};

} // namespace Mechanics

#endif // PT_GEOMETRY_H
