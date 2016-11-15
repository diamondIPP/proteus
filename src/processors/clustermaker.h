#ifndef PT_CLUSTERMAKER_H
#define PT_CLUSTERMAKER_H

#include "processor.h"

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

  void addNeighbours(const Storage::Hit* hit,
                     Storage::Plane* plane,
                     Storage::Cluster* cluster) const;
  void calculateCluster(Storage::Cluster* cluster) const;

public:
  ClusterMaker(unsigned int maxSeparationX,
               unsigned int maxSeparationY,
               double maxSeparation);

  void generateClusters(Storage::Event* event, unsigned int planeNum) const;

  std::string name() const;
  void process(Storage::Event& event) const;
  void finalize();
};
}

#endif // PT_CLUSTERMAKER_H
