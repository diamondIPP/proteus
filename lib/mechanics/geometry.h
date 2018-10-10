#ifndef PT_GEOMETRY_H
#define PT_GEOMETRY_H

#include <iosfwd>
#include <map>
#include <string>

#include <Eigen/SVD>

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
 *     r = r0 + Q * q ,
 *
 * and the reverse transformation as
 *
 *     q = R * (r - r0) = Q^T * (r - r0)  ,
 *
 * with r0 being the plane offset. Representing the plane orientation with
 * a rotation matrix allows for easy, direct calculations, but is not a
 * minimal set of parameters. The minimal set of six parameters contains only
 * three offsets and three rotation angles that define the rotation matrix.
 * Multiple conventions exists to define the three rotations. Here, the
 * 3-2-1 convention is used to build the rotation matrix as a product of
 * three elementary right-handed rotations
 *
 *     Q = R1(alpha) * R2(beta) * R3(gamma)  ,
 *
 * i.e. first a rotation by gamma around the local third axis
 * (normal axis), then a rotation by beta around the updated second
 * axis, followed with a rotation by alpha around the first axis.
 *
 * Alignment of a plane corresponds to the multiplication of an additional
 * rotation matrix and a change of the plane offset. This can be defined
 * either with respect to the local coordinate system
 *
 *     r  -> r' = r0 + Q * (dq + dQ * q) = r0' + Q' * q  ,
 *
 * or
 *
 *     q  -> q' = dR * R * (r - r0 - dr) = R' * (r - r0')  ,
 *
 * where the offset corrections dq0/dr0 and rotation corrections dQ/dR are
 * equivalent descriptions of the same change and can be converted into each
 * other as
 *
 *     r0 -> r0' = r0 + dr = r0 + Q * dq
 *     Q  ->  Q' = Q  * dQ =  Q   * dR^T
 *     R  ->  R' = dR * R  = dQ^T * Q^T  = (Q * dQ)^T = Q'^T  .
 *
 * The rotation correction matrix can again be expressed as composition of
 * three elementary rotations
 *
 *     dQ = R1(dalpha) * R2(dbeta) * R3(dgamma)  ,
 *
 * with small alignment angles (dalpha,dbeta,dgamma). Usually the alignment
 * angles are small enough to warant a linear approximation with results
 * in the following approximate correction matrix
 *
 *          |       1  -dgamma    dbeta |
 *     dQ = |  dgamma        1  -dalpha |  ,
 *          |  -dbeta   dalpha        1 |
 *
 * where the positive angles represent right-handed rotations around
 * the current local axes. By construction the equivalent correction matrix dR
 * in the global system can be described by the same rotation angles, but with
 * opposite signs due to the tranpose, i.e.
 *
 *          |      1    dgamma   -dbeta |
 *     dR = | -dgamma        1   dalpha | = dQ^T  .
 *          |   dbeta  -dalpha        1 |
 *
 * A fixed position r in global coordinates changes its local coordinates
 * after alignment as follows
 *
 *     q' = Q'^T * (r - r0')
 *        = dQ^T * Q^T * (r - r0 - dr)
 *        = dQ^T * Q^T * (r - r0) - dQ^T * Q^T * dr
 *        = dQ^T q - dq  .
 *
 */
class Plane {
public:
  /** Construct a plane consistent with the global system. */
  Plane() : m_rotation(Matrix3::Identity()), m_offset(Vector3::Zero()) {}
  /** Construct a plane from 3-2-1 rotation angles. */
  static Plane
  fromAngles321(double gamma, double beta, double alpha, const Vector3& offset);
  /** Construct a plane from two direction vectors for the local axes. */
  static Plane fromDirections(const Vector3& dirU,
                              const Vector3& dirV,
                              const Vector3& offset);

  /** Create a corrected plane from [dx, dy, dz, dalpha, dbeta, dgamma]. */
  Plane correctedGlobal(const Vector6& delta) const;
  /** Create a corrected plane from [du, dv, dw, dalpha, dbeta, dgamma]. */
  Plane correctedLocal(const Vector6& delta) const;

  auto rotationToGlobal() const { return m_rotation; }
  auto rotationToLocal() const { return m_rotation.transpose(); }
  auto unitU() const { return m_rotation.col(0); }
  auto unitV() const { return m_rotation.col(1); }
  auto unitNormal() const { return m_rotation.col(2); }
  auto offset() const { return m_offset; }
  /** Compute minimal parameters [x0, y0, z0, alpha, beta, gamma]. */
  Vector6 asParams() const;

  /** Transform a local position into global coordinates.
   *
   * For local positions with less than 3 coordinates the missing components
   * are assumed to be zero.
   */
  template <typename Position>
  auto toGlobal(const Eigen::MatrixBase<Position>& uvw) const
  {
    return m_offset + m_rotation.leftCols<Position::RowsAtCompileTime>() * uvw;
  }
  /** Transform a global position into local coordinates. */
  template <typename Position>
  auto toLocal(const Eigen::MatrixBase<Position>& xyz) const
  {
    return m_rotation.transpose() * (xyz - m_offset);
  }

private:
  template <typename Offset, typename Rotation>
  Plane(const Eigen::MatrixBase<Offset>& off,
        const Eigen::MatrixBase<Rotation>& rot)
      : m_rotation(rot), m_offset(off)
  {
    // ensure orthogonality of rotation matrix
    // https://en.wikipedia.org/wiki/Orthogonal_matrix#Nearest_orthogonal_matrix
    Eigen::JacobiSVD<Matrix3, Eigen::NoQRPreconditioner> svd(
        rot, Eigen::ComputeFullU | Eigen::ComputeFullV);
    m_rotation = svd.matrixU() * svd.matrixV().transpose();
  }

  Matrix3 m_rotation; // from local to global coordinates
  Vector3 m_offset;   // position of the origin in global coordinates
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
  /** Write geometry to a configuration file. */
  void writeFile(const std::string& path) const;

  /** Construct geometry from a configuration object. */
  static Geometry fromConfig(const toml::Value& cfg);
  /** Convert geometry into a configuration object. */
  toml::Value toConfig() const;

  /** Change the global offset by small values.
   *
   * \warning Does not update the associated covariance matrix.
   */
  void correctGlobalOffset(Index sensorId, double dx, double dy, double dz);
  /** Add small global corrections [dx, dy, dz, dalpha, dbeta, dgamma]. */
  void
  correctGlobal(Index sensorId, const Vector6& delta, const SymMatrix6& cov);
  /** Add small local corrections [du, dv, dw, dalpha, dbeta, dgamma]. */
  void
  correctLocal(Index sensorId, const Vector6& delta, const SymMatrix6& cov);

  /** The local sensor plane object. */
  const Plane& getPlane(Index sensorId) const;
  /** Geometry parameters [x, y, z, alpha, beta, gamma] for a sensor. */
  Vector6 getParams(Index sensorId) const;
  /** Geometry parameters covariance matrix. */
  SymMatrix6 getParamsCov(Index sensorId) const;

  /** Set the beam slope along the z axis. */
  void setBeamSlope(double slopeX, double slopeY);
  /** Set the beam divergence/ standard deviation along the z axis. */
  void setBeamDivergence(double divergenceX, double divergenceY);
  /** Beam energy. */
  double beamEnergy() const { return m_beamEnergy; }
  /** Beam slope in the global coordinate system. */
  Vector2 beamSlope() const { return {m_beamSlopeX, m_beamSlopeY}; }
  /** Beam slope divergence along the z axis in global x and y coordinates. */
  Vector2 beamDivergence() const { return {m_beamStdevX, m_beamStdevY}; }
  /** Beam direction in the local coordinate system of the sensor. */
  Vector2 getBeamSlope(Index sensorId) const;
  /** Beam slope covariance in the local coordinate system of the sensor. */
  SymMatrix2 getBeamCovariance(Index sensorId) const;

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  std::map<Index, Plane> m_planes;
  std::map<Index, SymMatrix6> m_covs;
  double m_beamSlopeX, m_beamSlopeY;
  double m_beamStdevX, m_beamStdevY;
  double m_beamEnergy;
};

/** Sort the sensor indices by their position along the beam direction. */
std::vector<Index> sortedAlongBeam(const Geometry& geo,
                                   const std::vector<Index>& sensorIds);

} // namespace Mechanics

#endif // PT_GEOMETRY_H
