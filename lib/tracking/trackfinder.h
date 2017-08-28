#ifndef PT_TRACKFINDER_H
#define PT_TRACKFINDER_H

#include <memory>
#include <vector>

#include "processors/processor.h"
#include "utils/definitions.h"

namespace Mechanics {
class Device;
}
namespace Storage {
class SensorEvent;
class Track;
}

namespace Tracking {

/** Find tracks assuming straight propagation along the beam direction.
 *
 * Matching clusters are searched for only on the selected sensors in the order
 * in which they are given. In case of ambiguities, the track bifurcates info
 * multiple candidates. Ambiguities are resolved after all track candidates
 * have been found by associating clusters exclusively to the best candidate,
 * i.e. the one with the highest number of hits and the lowest chi2 value, to
 * form a track. Successive candidates that contain clusters that are already
 * used are dropped.
 */
class TrackFinder : public Processors::Processor {
public:
  /**
   * \param numClustersMin Selection cut on number of required clusters
   * \param searchSigmaMax Association cut on clusters, negative to disable
   * \param redChi2Max Selection cut on chi2/d.o.f, negative to disable
   */
  TrackFinder(const Mechanics::Device& device,
              std::vector<Index> sensors,
              Index numClustersMin,
              double searchSigmaMax = -1,
              double redChi2Max = -1);

  std::string name() const;
  /** Find tracks and add them to the event. */
  void process(Storage::Event& event) const;

private:
  typedef std::unique_ptr<Storage::Track> TrackPtr;

  void searchSensor(Storage::SensorEvent& sensorEvent,
                    std::vector<TrackPtr>& candidates) const;
  void selectTracks(std::vector<TrackPtr>& candidates,
                    Storage::Event& event) const;

  std::vector<Index> m_sensorIds;
  Index m_numClustersMin;
  double m_d2Max;
  double m_redChi2Max;
  XYZVector m_beamDirection;
};

} // namespace Tracking

#endif // PT_TRACKFINDER_H
