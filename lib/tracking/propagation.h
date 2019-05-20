/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-10
 */

#pragma once

#include "storage/trackstate.h"
#include "utils/definitions.h"

namespace proteus {

class Plane;

/** Spatial slope [slope0,slope1] transport jacobian between two systems.
 *
 * \param tangent   Track tangent in the source local system
 * \param toTarget  Transformation from source local to the target local system
 */
Matrix2 jacobianSlopeSlope(const Vector4& tangent, const Matrix4& toTarget);

/** Full parameter transport jacobian between two systems.
 *
 * \param tangent   Initial track tangent in the source system
 * \param toTarget  Transformation from the source to the target system
 * \param w0        Initial distance to the plane along the target normal
 */
Matrix6
jacobianState(const Vector4& tangent, const Matrix4& toTarget, Scalar w0);

/** Propagate to the target plane and return the propagated state.
 *
 * \param state   Track state on the source plane
 * \param source  Source plane
 * \param target  Target plane
 */
TrackState
propagateTo(const TrackState& state, const Plane& source, const Plane& target);

} // namespace proteus
