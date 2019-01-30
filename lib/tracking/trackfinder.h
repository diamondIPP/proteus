#ifndef PT_TRACKFINDER_H
#define PT_TRACKFINDER_H

#include <memory>
#include <vector>

#include "loop/processor.h"
#include "utils/definitions.h"

namespace Mechanics {
class Device;
class Geometry;
} // namespace Mechanics
namespace Storage {
class SensorEvent;
class Track;
} // namespace Storage

namespace Tracking {

/** Find tracks assuming straight propagation along the beam direction.
 *
 * Matching clusters are searched for only on the selected sensors ordered
 * along the beam direction. In case of ambiguities, the track bifurcates info
 * multiple candidates. Ambiguities are resolved after all track candidates
 * have been found by associating clusters exclusively to the best candidate,
 * i.e. the one with the highest number of hits and the lowest chi2 value, to
 * form a track. Successive candidates that contain clusters that are already
 * used are dropped.
 */
class TrackFinder : public Loop::Processor {
public:
  /**
   * \param numClustersMin Selection cut on number of required clusters
   * \param searchSigmaMax Association cut on clusters, negative to disable
   * \param redChi2Max Selection cut on chi2/d.o.f, negative to disable
   */
  TrackFinder(const Mechanics::Device& device,
              const std::vector<Index>& sensors,
              const Index numClustersMin,
              const double searchSigmaMax = -1,
              const double redChi2Max = -1);

  std::string name() const;
  /** Find tracks and add them to the event. */
  void execute(Storage::Event& event) const;

private:
  using TrackPtr = std::unique_ptr<Storage::Track>;

  void searchSensor(Index sensorId,
                    Storage::SensorEvent& sensorEvent,
                    std::vector<TrackPtr>& candidates) const;
  void selectTracks(std::vector<TrackPtr>& candidates,
                    Storage::Event& event) const;

  const Mechanics::Geometry& m_geo;
  std::vector<Index> m_sensorIds;
  Index m_numClustersMin;
  double m_d2Max;
  double m_redChi2Max;
};

} // namespace Tracking

#endif // PT_TRACKFINDER_H
