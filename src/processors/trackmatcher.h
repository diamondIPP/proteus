#ifndef TRACKMATCHING_H
#define TRACKMATCHING_H

namespace Storage { class Event; }
namespace Storage { class Plane; }
namespace Storage { class Cluster; }
namespace Storage { class Track; }
namespace Mechanics { class Device; }
namespace Mechanics { class Sensor; }

namespace Processors {

  class TrackMatcher {
  public:
    TrackMatcher(const Mechanics::Device* device);
    
    void matchEvent(Storage::Event* refEvent,
		    Storage::Event* dutEvent);
    
  private:    
    void matchTracksToClusters(Storage::Event* trackEvent,
			       Storage::Plane* clustersPlane,
			       const Mechanics::Sensor* clustersSensor);

    void matchClustersToTracks(Storage::Event* trackEvent,
			       Storage::Plane* clustersPlane,
			       const Mechanics::Sensor* clustersSensor);

  private:
    const Mechanics::Device* _device;
  
  }; // end of class
  
} // end of namespace

#endif // TRACKMATCHING_H
