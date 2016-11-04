#ifndef PT_TRACKMAKER_H
#define PT_TRACKMAKER_H

#include <vector>

#include "processor.h"

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
  const double m_distMax;
  const unsigned int m_numSeedPlanes;
  const unsigned int m_nPointsMin;
  double m_beamSlopeX;
  double m_beamSlopeY;

  mutable Storage::Event* m_event;
  mutable int m_maskedPlane;

  void searchPlane(Storage::Track* track,
                   std::vector<Storage::Track*>& candidates,
                   unsigned int nplane) const;
};

} // namespace Processors

#endif // PT_TRACKMAKER_H
