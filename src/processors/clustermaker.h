#ifndef PT_CLUSTERMAKER_H
#define PT_CLUSTERMAKER_H

#include "processors/processor.h"

namespace Storage {
class Hit;
class Cluster;
class Plane;
class Event;
}

namespace Processors {

class ClusterMaker : public Processor {
public:
  ClusterMaker(int maxSeparationCol,
               int maxSeparationRow,
               double maxSeparationColRow);

  std::string name() const;
  void process(Storage::Event& event) const;

  void generateClusters(Storage::Event* event, unsigned int planeNum) const;

private:
  void addNeighbours(const Storage::Hit* hit,
                     Storage::Plane* plane,
                     Storage::Cluster* cluster) const;
  void calculateCluster(Storage::Cluster* cluster) const;

  int m_maxSeparationCol;
  int m_maxSeparationRow;
  double m_maxSeparationColRowSquared;
};

} // namespace Processors

#endif // PT_CLUSTERMAKER_H
