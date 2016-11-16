#ifndef PT_MATCHER_H
#define PT_MATCHER_H

#include <string>

#include "processors/processor.h"
#include "utils/definitions.h"

namespace Storage {
class Plane;
class Cluster;
class Track;
}
namespace Mechanics {
class Device;
class Sensor;
}

namespace Processors {

/** Match tracks and clusters on a sensor plane.
 *
 * This matches the closest track/cluster pair together. The track must have
 * a local state on the selected sensor to be considered for matching. The
 * matching is unique, i.e. every track and every cluster is matched at most
 * once.
 */
class Matcher : public Processor {
public:
  Matcher(const Mechanics::Device& device, Index sensorId);

  std::string name() const;
  void process(Storage::Event& event) const;

private:
  const Mechanics::Sensor& m_sensor;
  Index m_sensorId;
  std::string m_name;
};

/** \deprecated Judith-style track/cluster matcher. */
class TrackMatcher {
public:
  TrackMatcher(const Mechanics::Device* device);

  void matchEvent(Storage::Event* refEvent, Storage::Event* dutEvent);

private:
  void matchTracksToClusters(Storage::Event* trackEvent,
                             Storage::Plane* clustersPlane,
                             const Mechanics::Sensor* clustersSensor);
  void matchClustersToTracks(Storage::Event* trackEvent,
                             Storage::Plane* clustersPlane,
                             const Mechanics::Sensor* clustersSensor);

  const Mechanics::Device* _device;
};

} // namespace Processors

#endif // PT_MATCHER_H
