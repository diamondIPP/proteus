#ifndef PT_TRACKFINDER_H
#define PT_TRACKFINDER_H

#include <vector>

#include "loop/processor.h"
#include "mechanics/geometry.h"
#include "utils/definitions.h"

namespace proteus {

class Device;
class SensorEvent;
class Track;

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
class TrackFinder : public Processor {
public:
  /**
   * \param trackingIds            Ids of tracking sensors
   * \param searchSpatialSigmaMax  Spatial search cut, negative to disable
   * \param searchTemporalSigmaMax Temporal search cut, negative to disable
   * \param sizeMin                Selection cut on number of clusters
   * \param redChi2Max             Cut on track chi2/d.o.f, negative to disable
   */
  TrackFinder(const Device& device,
              std::vector<Index> trackingIds,
              double searchSpatialSigmaMax,
              double searchTemporalSigmaMax,
              size_t sizeMin,
              double redChi2Max);

  std::string name() const;
  /** Find tracks and add them to the event. */
  void execute(Event& event) const;

private:
  struct Step {
    // Copy of the local-global transformation to avoid lookup
    Plane plane;
    // Propagation uncertainty, e.g. from scattering
    SymMatrix6 processNoise = SymMatrix6::Zero();
    // Corresponding sensor
    Index sensorId = kInvalidIndex;
    bool useForTracking = false;
    bool useForSeeding = false;
    // Mininum size of viable candidates after this step
    size_t candidateSizeMin = 0;
    // Initial direction for seeds generated during this step
    Vector2 seedSlope = Vector2::Zero();
    SymMatrix2 seedSlopeCovariance = SymMatrix2::Zero();

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  };
  std::vector<Step> m_steps;
  double m_d2LocMax;
  double m_d2TimeMax;
  double m_reducedChi2Max;
};

} // namespace proteus

#endif // PT_TRACKFINDER_H
