#ifndef PT_TRACKFINDER_H
#define PT_TRACKFINDER_H

#include <vector>

#include "loop/processor.h"
#include "mechanics/geometry.h"
#include "utils/definitions.h"

namespace Mechanics {
class Device;
} // namespace Mechanics
namespace Storage {
class SensorEvent;
class Track;
} // namespace Storage

namespace Tracking {

/** Find tracks assuming straight propagation along the beam direction.
 *
 * Matching clusters are searched for only on the selected sensors ordered
 * along the beam direction. In case of ambiguities, the track bifurcates into
 * multiple candidates. Ambiguities are resolved after all track candidates
 * have been found by associating clusters exclusively to the best candidate,
 * i.e. the one with the highest number of hits and the lowest chi2 value, to
 * form a track. Successive candidates that contain clusters that are already
 * used are dropped.
 *
 * The ``Tracks``s build by the track finder store the constituent clusters
 * and an estimate of the global track parameters. Local track states are
 * not estimated and must be computed using one of the fitter processors.
 */
class TrackFinder : public Loop::Processor {
public:
  /**
   * \param trackingIds     Ids of sensors that should be used for tracking
   * \param sizeMin         Selection cut on number of clusters
   * \param searchSigmaMax  Selection cut on clusters, negative to disable
   * \param redChi2Max      Selection cut on chi2/d.o.f, negative to disable
   */
  TrackFinder(const Mechanics::Device& device,
              std::vector<Index> trackingIds,
              size_t sizeMin = 0,
              double searchSigmaMax = -1,
              double redChi2Max = -1);

  std::string name() const;
  /** Find tracks and add them to the event. */
  void execute(Storage::Event& event) const;

private:
  struct Step {
    Index isensor = kInvalidIndex;
    bool useForTracking = false;
    bool useForSeeding = false;
    size_t remainingTrackingSteps;
    Mechanics::Plane plane;
    SymMatrix6 processNoise = SymMatrix6::Zero();
    Vector2 beamSlope = Vector2::Zero();
    SymMatrix2 beamSlopeCovariance = SymMatrix2::Zero();

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  };
  std::vector<Step> m_steps;
  size_t m_sizeMin;
  double m_d2Max;
  double m_reducedChi2Max;
};

} // namespace Tracking

#endif // PT_TRACKFINDER_H
