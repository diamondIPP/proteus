#ifndef PT_TRACKING_H
#define PT_TRACKING_H

#include "storage/track.h"
#include "utils/definitions.h"

namespace Mechanics {
class Geometry;
}

namespace Tracking {

/** Fit a straight track in the global coordinates.
 *
 * \param[in,out] track The global track state is set to fit result.
 */
void fitTrackGlobal(Storage::Track& track);

/** Fit a straight track in the local reference coordinates.
 *
 * \param[in] track Only the track clusters are used.
 * \param[in] geo Detector geometry with local-to-global transformations.
 * \param[in] referenceId Sensor index corresponding to the reference sensor.
 * \return Fitted state in local coordinates on the reference sensor.
 */
Storage::TrackState fitTrackLocal(const Storage::Track& track,
                                  const Mechanics::Geometry& geo,
                                  Index referenceId);

/** Fit a straight track ignoring the measurement on the reference sensor.
 *
 * \param[in] track Only the track clusters are used.
 * \param[in] geo Detector geometry with local-to-global transformations.
 * \param[in] referenceId Sensor index corresponding to the reference sensor.
 * \return Fitted state in local coordinates on the reference sensor.
 */
Storage::TrackState fitTrackLocalUnbiased(const Storage::Track& track,
                                          const Mechanics::Geometry& geo,
                                          Index referenceId);

} // namespace Tracking

#endif // PT_TRACKING_H
