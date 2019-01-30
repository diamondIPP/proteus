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

/** Spatial slope [slope0,slope1] transport jacobian between two systems.
 *
 * \param tangent   Track tangent in the source local system
 * \param toTarget  Transformation from source local to the target local system
 */
Matrix2 jacobianSlopeSlope(const Vector4& tangent, const Matrix4& toTarget);

/** Full parameter transport jacobian between two systems.
 *
 * \param tangent   Track tangent in the source local system
 * \param toTarget  Transformation from source local to the target local system
 * \param w         Propagation distance along the target normal direction
 */
Matrix6
jacobianState(const Vector4& tangent, const Matrix4& toTarget, Scalar w);

/** Propagate to the target plane and return the propagated state.
 *
 * \param state   Track state on the source plane
 * \param source  Source plane
 * \param target  Target plane
 */
Storage::TrackState propagate_to(const Storage::TrackState& state,
                                 const Mechanics::Plane& source,
                                 const Mechanics::Plane& target);

} // namespace Tracking

#endif // PT_PROPAGATION_H
