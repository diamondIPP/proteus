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
             unsigned int minClusters = 3,
             bool calcIntercepts = false);

  void generateTracks(Storage::Event* event,
                      double beamAngleX = 0,
                      double beamAngleY = 0,
                      int maskedPlane = -1) const;

  bool getCalcIntercepts() { return m_calcIntercepts; }
  std::string name() const;
  void process(Storage::Event& event) const;

private:
  const double m_distMax;
  const unsigned int m_numSeedPlanes;
  const unsigned int m_nPointsMin;
  mutable double m_beamSlopeX;
  mutable double m_beamSlopeY;

  mutable Storage::Event* m_event;
  mutable int m_maskedPlane;
  bool m_calcIntercepts;

  void searchPlane(Storage::Track* track,
                   std::vector<Storage::Track*>& candidates,
                   unsigned int nplane) const;
};

} // namespace Processors

#endif // PT_TRACKMAKER_H
