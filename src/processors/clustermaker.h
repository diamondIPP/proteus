#ifndef CLUSTERMAKER_H
#define CLUSTERMAKER_H

#include "processors.h"

namespace Storage {
class Hit;
class Cluster;
class Plane;
class Event;
}

namespace Processors {

class ClusterMaker : public Processor {
private:
  const unsigned int _maxSeparationX;
  const unsigned int _maxSeparationY;
  const double _maxSeparation;

  void addNeighbours(const Storage::Hit* hit, Storage::Plane* plane,
                     Storage::Cluster* cluster);
  void calculateCluster(Storage::Cluster* cluster);

public:
  ClusterMaker(unsigned int maxSeparationX, unsigned int maxSeparationY,
               double maxSeparation);

  void generateClusters(Storage::Event* event, unsigned int planeNum);

  void processEvent(Storage::Event* event);
  void finalize();
};
}

#endif  // CLUSTERMAKER_H
