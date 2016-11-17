#ifndef PT_TRACKING_H
#define PT_TRACKING_H

#include "storage/track.h"
#include "utils/definitions.h"

namespace Mechanics {
class Sensor;
}

namespace Processors {

/** Fit track to clusters using a simple straight line fit.
 *
 * \param[in,out] track The global track state is set to fit result.
 */
void fitTrack(Storage::Track& track);

/** Fit a straight track in the local reference coordinates.
 *
 * \param[in] track Only the track clusters are used.
 * \param[in] reference Reference sensor that defines the local coordinates.
 * \return Fitted state in local coordinates on the reference sensor.
 */
Storage::TrackState fitTrackLocal(const Storage::Track& track,
                                  const Mechanics::Sensor& reference);

/** Fit track to clusters ignoring any measurements on the reference sensor.
 *
 * \param[in] track Only the track clusters are used.
 * \param[in] reference Reference sensor that defines the local coordinates.
 * \param[in] referenceId Sensor index corresponding to the reference sensor.
 * \return Fitted state in local coordinates on the reference sensor.
 */
Storage::TrackState fitTrackLocalUnbiased(const Storage::Track& track,
                                          const Mechanics::Sensor& reference,
                                          Index referenceId);

} // namespace Processors

#endif // PT_TRACKING_H
