#ifndef PT_TRACKFINDER_H
#define PT_TRACKFINDER_H

#include <memory>
#include <vector>

#include "processor.h"
#include "utils/definitions.h"

namespace Mechanics {
class Device;
}
namespace Storage {
class Track;
class Plane;
}

namespace Processors {

/** Find tracks assuming straight propagation along the beam direction.
 *
 * Matching clusters are searched for only on the selected sensors in the order
 * in which they are given. In case of ambiguities, the track bifurcates info
 * multiple candidates. Ambiguities are resolved after all track candidates
 * have been found. Clusters are associated exclusively to the best track, i.e.
 * the one with the highest number of hits and the lowest chi2 value.
 */
class TrackFinder : public Processor {
public:
  TrackFinder(const Mechanics::Device& device,
              const std::vector<int> sensors,
              Index clustersMin,
              double distanceSigmaMax);

  std::string name() const;
  /** Find tracks and add them to the event. */
  void process(Storage::Event& event) const;

private:
  typedef std::unique_ptr<Storage::Track> TrackPtr;

  void searchSensor(Storage::Plane& sensorEvent,
                    std::vector<TrackPtr>& candidates) const;
  void selectTracks(std::vector<TrackPtr>& candidates,
                    Storage::Event& event) const;

  std::vector<Index> m_seeding;
  std::vector<Index> m_tracking;
  Index m_clustersMin;
  double m_distSigmaMax;
  XYZVector m_beamDirection;
};

} // namespace Processors

#endif // PT_TRACKFINDER_H
