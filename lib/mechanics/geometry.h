// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#pragma once

#include <iosfwd>
#include <map>
#include <string>

#include <Eigen/SVD>

#include "utils/config.h"
#include "utils/definitions.h"

namespace proteus {

/** A two-dimensional plane in three-dimensional space with time.
 *
 * The plane is defined by two internal, orthogonal axes, and the origin of
 * the plane in the global coordinates. The normal direction to the plane is
 * defined by the additional third orthogonal vector in the right-handed
 * coordinate system. A fourth coordinate measures time.
 *
 * The unit vectors corresponding to the internal axes and the normal direction
 * are the columns of the local-to-global linear transformation matrix Q.
 * The transformation from local coordinates q to global coordinates r
 * follows as
 *
 *     r = r0 + Q * q ,
 *
 * and the reverse transformation as
 *
 *     q = R * (r - r0) = Q^T * (r - r0)  ,
 *
 * with r0 being the local origin, both spatial and temporal, in global
 * coordinates, and s,t being the temporal coordinates in the local and global
 * system respectively. Representing the plane orientation with a linear
 * transformation matrix allows for easy, direct calculations, but is not a
 * minimal set of parameters. The minimal set of six parameters (seven if an
 * additional time offset is considered) contains only three offsets and three
 * rotation angles that define the rotation matrix.
 * Multiple conventions exists to define the three rotations. Here, the
 * 3-2-1 convention is used to define the rotation matrix as a product of
 * three elementary right-handed rotations
 *
 *     Q = R1(alpha) * R2(beta) * R3(gamma)  ,
 *
 * i.e. first a rotation by gamma around the local third spatial axis
 * (normal axis), then a rotation by beta around the updated second spatial
 * axis, followed with a rotation by alpha around the first spatial axis.
 *
 * Alignment of a plane corresponds to the multiplication of an additional
 * rotation matrix and a change of the plane offset. This can be defined
 * either with respect to the local coordinate system
 *
 *     r  -> r' = r0 + Q * (dq + dQ * q) = r0' + Q' * q  ,
 *
 * or with respect to the global coordinate system
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
 * angles are small enough to warant a linear approximation which results
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
 */
class Plane {
public:
  /** Construct a plane consistent with the global system. */
  Plane() : m_origin(Vector4::Zero()), m_linear(Matrix4::Identity()) {}
  /** Construct a plane from 3-2-1 rotation angles. */
  static Plane
  fromAngles321(double gamma, double beta, double alpha, const Vector3& origin);
  /** Construct a plane from two direction vectors for the local axes. */
  static Plane fromDirections(const Vector3& dirU,
                              const Vector3& dirV,
                              const Vector3& origin);

  /** Create a corrected plane from [dx, dy, dz, dalpha, dbeta, dgamma]. */
  Plane correctedGlobal(const Vector6& delta) const;
  /** Create a corrected plane from [du, dv, dw, dalpha, dbeta, dgamma]. */
  Plane correctedLocal(const Vector6& delta) const;

  const Vector4& origin() const { return m_origin; }
  const Matrix4& linearToGlobal() const { return m_linear; }
  auto linearToLocal() const { return m_linear.transpose(); }
  /** Compute minimal parameters [x0, y0, z0, alpha, beta, gamma]. */
  Vector6 asParams() const;

  /** Transform a local position into global coordinates. */
  template <typename Position>
  auto toGlobal(const Eigen::MatrixBase<Position>& local) const
  {
    return m_origin + m_linear * local;
  }
  /** Transform a global position into local coordinates. */
  template <typename Position>
  auto toLocal(const Eigen::MatrixBase<Position>& global) const
  {
    return m_linear.transpose() * (global - m_origin);
  }

private:
  template <typename Offset, typename Linear>
  Plane(const Eigen::MatrixBase<Offset>& origin,
        const Eigen::MatrixBase<Linear>& linear)
      : m_origin(origin), m_linear(linear)
  {
    // ensure orthogonality of the linear transformation matrix
    // https://en.wikipedia.org/wiki/Orthogonal_matrix#Nearest_orthogonal_matrix
    Eigen::JacobiSVD<Matrix4, Eigen::NoQRPreconditioner> svd(
        linear, Eigen::ComputeFullU | Eigen::ComputeFullV);
    m_linear = svd.matrixU() * svd.matrixV().transpose();
  }

  Vector4 m_origin; // position of the origin in global coordinates
  Matrix4 m_linear; // from local to global coordinates

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
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
  void setBeamSlope(const Vector2& slopeXY) { m_beamSlope = slopeXY; }
  /** Set the beam divergence/ standard deviation along the z axis. */
  void setBeamDivergence(const Vector2& divXY) { m_beamSlopeStdev = divXY; }
  /** Beam energy. */
  auto beamEnergy() const { return m_beamEnergy; }
  /** Beam slope in the global coordinate system. */
  auto beamSlope() const { return m_beamSlope; }
  /** Beam slope covariance in the global coordinate system. */
  SymMatrix2 beamSlopeCovariance() const;
  /** Beam direction in the local coordinate system of the sensor. */
  Vector2 getBeamSlope(Index sensorId) const;
  /** Beam slope covariance in the local coordinate system of the sensor. */
  SymMatrix2 getBeamSlopeCovariance(Index sensorId) const;

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  /** Beam tangent in the global coordinate system. */
  Vector4 beamTangent() const;

  std::map<Index, Plane> m_planes;
  std::map<Index, SymMatrix6> m_covs;
  Vector2 m_beamSlope;
  Vector2 m_beamSlopeStdev;
  Scalar m_beamEnergy;

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

/** Sort the sensor indices by their position along the beam direction. */
void sortAlongBeam(const Geometry& geo, std::vector<Index>& sensorIds);
/** Return a copy of the indices sorted along the beam direction. */
std::vector<Index> sortedAlongBeam(const Geometry& geo,
                                   const std::vector<Index>& sensorIds);

} // namespace proteus
