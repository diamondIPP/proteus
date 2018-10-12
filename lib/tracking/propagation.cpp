/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-10
 */

#include "propagation.h"

#include "utils/definitions.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Propagation)

// Jacobian from slope [u', v'] to normalized direction [tu, tv, tw]
//
// \param t Normalized direction vector
//
// The tangent vector is [u'/f, v'/f, 1/f] with f = sqrt(1 + u'^2 + v'^2)
template <typename Tangent>
static inline Matrix32 jacobianSlopeTangent(const Eigen::MatrixBase<Tangent>& t)
{
  Matrix32 jac;
  jac(0, 0) = (1 - t[0] * t[0]) * t[2];
  jac(1, 0) = -t[0] * t[1] * t[2];
  jac(2, 0) = -t[0] * t[2] * t[2];
  jac(0, 1) = -t[0] * t[1] * t[2];
  jac(1, 1) = (1 - t[1] * t[1]) * t[2];
  jac(2, 1) = -t[1] * t[2] * t[2];
  return jac;
}

// Jacobian from normalized direction [tu, tv, tw] to slope [u', v']
//
// \param t Normalized direction vector
//
// The slope vector is [tu / tw, tv / tw]
template <typename Tangent>
static inline Matrix23 jacobianTangentSlope(const Eigen::MatrixBase<Tangent>& t)
{
  Matrix23 jac;
  jac(0, 0) = 1 / t[2];
  jac(1, 0) = 0;
  jac(0, 1) = 0;
  jac(1, 1) = 1 / t[2];
  jac(0, 2) = -t[0] / (t[2] * t[2]);
  jac(1, 2) = -t[1] / (t[2] * t[2]);
  return jac;
}

Matrix2 Tracking::jacobianSlopeSlope(const Vector3& direction,
                                     const Matrix3& rotation)
{
  // local and global tangent unit vector
  auto tan0 = direction.normalized();
  auto tan1 = rotation * tan0;
  auto A = jacobianSlopeTangent(tan0);
  auto D = jacobianTangentSlope(tan1);
  // target slope <- target tangent <- source tangent <- source slope
  auto jac = D * rotation * A;
  DEBUG("tangent source: [", tan0[0], ", ", tan0[1], ", ", tan0[2], "]");
  DEBUG("tangent target: [", tan1[0], ", ", tan0[1], ", ", tan1[2], "]");
  DEBUG("matrix A:\n", A);
  DEBUG("matrix D:\n", D);
  DEBUG("jacobian:\n", jac);
  return jac;
}

Matrix4 Tracking::jacobianState(const Vector3& direction,
                                const Matrix3& rotation)
{
  // WARNING
  // current implementation assumes vanishing slopes and needs to be amended.
  // TODO 2017-10-10 msmk add full jacobian for all parameters

  Matrix4 jac;
  // clang-format off
  jac << rotation.topLeftCorner<2,2>(), Matrix2::Zero(),
         Matrix2::Zero(),               jacobianSlopeSlope(direction, rotation);
  // clang-format on

  return jac;
}

Storage::TrackState Tracking::propagate_to(const Storage::TrackState& state,
                                           const Mechanics::Plane& source,
                                           const Mechanics::Plane& target)
{
  // combined rotation from source to target system
  Matrix3 combinedRotation =
      target.rotationToLocal() * source.rotationToGlobal();
  // Transform to target local coordinates
  Vector3 uvw = target.toLocal(source.toGlobal(state.offset()));
  Vector3 dir = combinedRotation * state.direction();
  // Move to intersection w/ the target plane
  uvw -= dir * uvw[2] / dir[2];

  Matrix4 jac = jacobianState(state.direction(), combinedRotation);

  Storage::TrackState propagated(uvw.head<2>(), dir.head<2>() / dir[2]);
  propagated.setCov(transformCovariance(jac, state.cov()));
  return propagated;
}
