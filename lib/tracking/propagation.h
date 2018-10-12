/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-10
 */

#ifndef PT_PROPAGATION_H
#define PT_PROPAGATION_H

#include <utility>

#include "mechanics/geometry.h"
#include "storage/trackstate.h"

namespace Tracking {

/** Transport jacobian of the track slope between two coordinate systems.
 *
 * \param direction  Direction vector in the source system
 * \param rotation   Rotation matrix from the source to the target system
 */
Matrix2 jacobianSlopeSlope(const Vector3& direction, const Matrix3& rotation);

/** Transport jacobian of the full track state between two coordinate systems.
 *
 * \param direction  Direction vector in the source system
 * \param rotation   Rotation matrix from the source to the target system
 */
Matrix4 jacobianState(const Vector3& direction, const Matrix3& rotation);

/** Propagate to the target plane and return the propagated state.
 *
 * \param state   Track state on the source plane
 * \param source  Source plane
 * \param target  Target plane
 *
 * \warning Only propagates offset uncertainty for now.
 */
Storage::TrackState propagate_to(const Storage::TrackState& state,
                                 const Mechanics::Plane& source,
                                 const Mechanics::Plane& target);

} // namespace Tracking

#endif // PT_PROPAGATION_H
