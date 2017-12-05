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

/** Slope jacobian from one coordinate to another.
 *
 * \param direction  Direction vector in the initial system
 * \param rotation   Rotation matrix from the intial to the target system
 */
Matrix2 jacobianSlopeSlope(const Vector3& direction,
                           const Matrix3& rotation);

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