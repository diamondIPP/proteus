/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-10
 */

#include "propagation.h"

#include "utils/definitions.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Propagation)

Matrix2 Tracking::jacobianSlopeSlope(const Vector3& direction,
                                     const Matrix3& rotation)
{
  // local and global tangent unit vector
  Vector3 tl = direction.normalized();
  Vector3 tg = rotation * tl;
  DEBUG("tangent source: [", tl, "]");
  DEBUG("tangent target: [", tg, "]");
  // local slope to local tangent: [u', v'] to [tw * u', tw * v', tw]
  Matrix32 a;
  a(0, 0) = (1 - tl[0] * tl[0]) * tl[2];
  a(1, 0) = -tl[0] * tl[1] * tl[2];
  a(2, 0) = -tl[0] * tl[2] * tl[2];
  a(0, 1) = -tl[0] * tl[1] * tl[2];
  a(1, 1) = (1 - tl[1] * tl[1]) * tl[2];
  a(2, 1) = -tl[1] * tl[2] * tl[2];
  DEBUG("matrix a:\n", a);
  // global tangent to global slope: [tx, ty, tz] to [tx/tz, ty/tz]
  Matrix23 d;
  d(0, 0) = 1 / tg[2];
  d(1, 0) = 0;
  d(0, 1) = 0;
  d(1, 1) = 1 / tg[2];
  d(0, 2) = -tg[0] / (tg[2] * tg[2]);
  d(1, 2) = -tg[1] / (tg[2] * tg[2]);
  DEBUG("matrix d:\n", d);
  // jacobian: local slope -> local tangent -> global tangent -> global slope
  Matrix2 jac = d * rotation * a;
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
  // Transform to target local coordinates
  Vector3 uvw = target.toLocal(source.toGlobal(state.offset()));
  Vector3 dir =
      Transpose(target.rotation) * source.rotation * state.direction();
  // Move to intersection w/ the target plane
  uvw -= dir * uvw[2] / dir[2];

  // calculate offset jacobian.
  // WARNING assumes vanishing slopes and needs to be amended.
  Matrix2 jacOffset = Transpose(target.rotation.Sub<Matrix32>(0, 0)) *
                      source.rotation.Sub<Matrix32>(0, 0);
  // TODO 2017-10-10 msmk add full jacobian for all parameters

  Storage::TrackState propagated(uvw.Sub<Vector2>(0), dir);
  propagated.setCovOffset(Similarity(jacOffset, state.covOffset()));
  return propagated;
}
