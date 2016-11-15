#ifndef PT_MATCHER_H
#define PT_MATCHER_H

namespace Storage {
class Event;
class Plane;
class Cluster;
class Track;
}
namespace Mechanics {
class Device;
class Sensor;
}

namespace Processors {

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
