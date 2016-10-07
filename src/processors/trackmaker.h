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
private:
  const double _maxClusterDist;
  const unsigned int _numSeedPlanes;
  const unsigned int _minClusters;
  mutable double _beamAngleX;
  mutable double _beamAngleY;

  mutable Storage::Event* _event;
  mutable int _maskedPlane;
  bool _calcIntercepts;

  void searchPlane(Storage::Track* track,
                   std::vector<Storage::Track*>& candidates,
                   unsigned int nplane) const;

public:
  TrackMaker(double maxClusterDist,
             unsigned int numSeedPlanes = 1,
             unsigned int minClusters = 3,
             bool calcIntercepts = false);

  void generateTracks(Storage::Event* event, double beamAngleX = 0,
                      double beamAngleY = 0, int maskedPlane = -1) const;

  static int linearFit(const unsigned int npoints, const double* independant,
                       const double* dependant, const double* uncertainty,
                       double& slope, double& slopeErr, double& intercept,
                       double& interceptErr, double& chi2, double& covariance);

  static void fitTrackToClusters(Storage::Track* track);

  bool getCalcIntercepts() { return _calcIntercepts; }
  std::string name() const;
  void process(Storage::Event& event) const;
};

}  // namespace Processors

#endif  // PT_TRACKMAKER_H
