/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-10
 */

#include "propagation.h"

#include "utils/definitions.h"

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
