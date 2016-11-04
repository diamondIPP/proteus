#ifndef PT_TRACKMAKER_H
#define PT_TRACKMAKER_H

#include <vector>

#include "processor.h"
#include "utils/definitions.h"

namespace Storage {
class Event;
class Cluster;
class Track;
}
namespace Mechanics {
class Device;
class Sensor;
}

namespace Processors {

class TrackMaker : public Processor {
public:
  TrackMaker(double maxClusterDist,
             unsigned int numSeedPlanes = 1,
             unsigned int minClusters = 3);

  void setBeamSlope(double slopeX, double slopeY);

  std::string name() const;
  void process(Storage::Event& event) const;
  void generateTracks(Storage::Event* event, int maskedPlane = -1) const;

private:
  double m_distMax;
  Index m_numSeedPlanes;
  Index m_numPointsMin;
  double m_beamSlopeX;
  double m_beamSlopeY;

  mutable int m_maskedPlane;

  void searchPlane(Storage::Event* event,
                   Storage::Track* track,
                   std::vector<Storage::Track*>& candidates,
                   unsigned int nplane) const;
};

} // namespace Processors

#endif // PT_TRACKMAKER_H
