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
 * The plane is defined by two internal, orthogonal axes, and the origin of
 * the plane in the global coordinates. The normal direction to the plane is
 * already defined by its two internal axes.
 *
 * The unit vectors corresponding to the internal axes and the normal direction
 * are the columns of the local-to-global rotation matrix Q. The transformation
 * from local coordinates q=(u,v,w) to global coordinates r=(x,y,z) follows
 * as
 *
 *     r = r_0 + Q * q ,
 *
 * with r_0 being the plane offset. Representing the plane orientation with
 * a rotation matrix allows for easy, direct calculations, but is not a
 * minimal set of parameters. The minimal set of six parameters contains only
 * three offsets and three rotation angles that define the rotation matrix.
 * Multiple conventions exists to define the three rotations. Here, the
 * 3-2-1 convention is used to build the rotation matrix as a product of
 * three elementary rotations
 *
 *     Q = R_1(a1) * R_2(a2) * R_3(a3) ,
 *
 * i.e. first a rotation by a3 around the local third axis
 * (normal axis), then a rotation by a2 around the updated second
 * axis, followed with a rotation by a1 around the first axis.
 */
struct Plane {
  Matrix3 rotation; // from local to global coordinates
  Vector3 offset;   // position of the origin in global coordinates

  static Plane
  fromAnglesZYX(double rotZ, double rotY, double rotX, const Vector3& offset);
  static Plane fromDirections(const Vector3& dirU,
                              const Vector3& dirV,
                              const Vector3& offset);

  /** Compute the equivalent local-to-global Transform3D object. */
  Transform3D asTransform3D() const;
  /** Compute geometry parameters [x0, y0, z0, a1, a2, a3]. */
  Vector6 asParams() const;

  Vector3 unitU() const { return rotation.SubCol<Vector3>(0); }
  Vector3 unitV() const { return rotation.SubCol<Vector3>(1); }
  Vector3 unitNormal() const { return rotation.SubCol<Vector3>(2); }

  /** Transform a global position into local coordinates. */
  Vector3 toLocal(const Vector3& xyz) const;
  /** Transform a global position into local coordinates. */
  Vector3 toLocal(const XYZPoint& xyz) const;
  /** Transform a local position on the plane into global coordinates. */
  Vector3 toGlobal(const Vector2& uv) const;
  /** Transform a local position on the plane into global coordinates. */
  Vector3 toGlobal(const XYPoint& uv) const;
  /** Transform a local position into global coordinates. */
  Vector3 toGlobal(const Vector3& uvw) const;
};

/** Store and process the geometry of the telescope setup.
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

  /** The local sensor plane object. */
  const Plane& getPlane(Index sensorId) const;
  /** Transformation from local to global coordinates for the sensor. */
  Transform3D getLocalToGlobal(Index sensorId) const;
  /** Geometry parameters [x, y, z, alpha, beta, gamma] for a sensor. */
  Vector6 getParams(Index sensorId) const;
  /** Geometry parameters covariance matrix. */
  SymMatrix6 getParamsCov(Index sensorId) const;

  void setBeamSlope(double slopeX, double slopeY);
  /** Beam direction in the global coordinate system. */
  Vector3 beamDirection() const;

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  std::map<Index, Plane> m_planes;
  std::map<Index, SymMatrix6> m_covs;
  double m_beamSlopeX, m_beamSlopeY;
};

/** Sort the sensor indices by their position along the beam direction. */
std::vector<Index> sortedAlongBeam(const Geometry& geo,
                                   const std::vector<Index>& sensorIds);

} // namespace Mechanics

#endif // PT_GEOMETRY_H
