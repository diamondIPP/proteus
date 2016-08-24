#include "clustermaker.h"

#include <cassert>
#include <math.h>
#include <float.h>

#include "processors.h"
#include "storage/plane.h"
#include "storage/cluster.h"
#include "storage/hit.h"
#include "storage/event.h"

namespace Processors {

void ClusterMaker::addNeighbours(const Storage::Hit* hit, Storage::Plane* plane,
                                  Storage::Cluster* cluster)
{
  // Go through all hits
  for (unsigned int nhit = 0; nhit < plane->numHits(); nhit++)
  {
    Storage::Hit* compare = plane->getHit(nhit);

    // Continue if this hit is already clustered or if it is the one being considered
    if (compare->isInCluster() || compare == hit) continue;

    // If a maximum separation has been defined in real coordinates, check now
    if (_maxSeparation > 0)
    {
      const double distX = compare->getPosX() - hit->getPosX();
      const double distY = compare->getPosY() - hit->getPosY();
      const double dist = sqrt(pow(distX, 2) + pow(distY, 2));
      if (dist > _maxSeparation) continue;
    }
    else
    {
   //   std::cout<< _maxSeparationX<<std::endl;
      const int distX = compare->getPixX() - hit->getPixX();
      const int distY = compare->getPixY() - hit->getPixY();
      if (fabs(distX) > _maxSeparationX || fabs(distY) > _maxSeparationY)
        continue;
    }

    // Add this hit to the cluster we are making
    cluster->addHit(compare);

    // Continue adding neigbours of this hit into the cluster
    addNeighbours(compare, plane, cluster);
  }
}

void ClusterMaker::generateClusters(Storage::Event* event, unsigned int planeNum)
{
   Storage::Plane* plane = event->getPlane(planeNum);
  if (plane->numClusters() > 0)
    throw "ClusterMaker: clusters already exist for this hit";

  for (unsigned int nhit = 0; nhit < plane->numHits(); nhit++)
  {
     Storage::Hit* hit = plane->getHit(nhit);

    // If the hit isn't clustered, make a new cluster
    if (!hit->isInCluster()) {
       Storage::Cluster* cluster = plane->newCluster();
      cluster->addHit(hit);
    }

    // Add neighbouring clusters to this hit (this is recursive)
     Storage::Cluster* lastCluster = plane->getCluster(plane->numClusters() - 1);
    assert(lastCluster && "ClusterMaker: hits didn't generate any clusters");
    addNeighbours(hit, plane, lastCluster);
  }

  // Recursive search has ended, finalize all the cluster information
  for (unsigned int i = 0; i < plane->numClusters(); i++)
    calculateCluster(plane->getCluster(i));
}

void ClusterMaker::calculateCluster(Storage::Cluster* cluster)
{
  const double rt12 = 1.0 / sqrt(12);
  double pixErrX = rt12;
  double pixErrY = rt12;

  double cogX = 0;
  double cogY = 0;
  double weight = 0;
  double fastest_timing = 100;
  double fastest_hitX=0, fastest_hitY=0;

  for (unsigned int nhit = 0; nhit < cluster->getNumHits(); nhit++)
  {
    const Storage::Hit* hit = cluster->getHit(nhit);
    //  const double value = (hit->getValue() > 0) ? hit->getValue() : 1;
    //taking the fastest hit to build the cluster
    //digital sensor:
    cogX += hit->getPixX();
    cogY += hit->getPixY();
    weight += 1;

    if(fastest_timing>hit->getTiming()){fastest_timing=hit->getTiming(); fastest_hitX=hit->getPixX(); fastest_hitY=hit->getPixY();    }


  //    cogX += hit->getPixX() * value;
    //    cogY += hit->getPixY() * value;
    //    weight += hit->getValue();
  }

  


 // cogX /= weight;
 // cogY /= weight;
 cogX=fastest_hitX;
 cogY=fastest_hitY;

  double stdevX = 0;
  double stdevY = 0;
  double weight2 = 0;

  for (unsigned int nhit = 0; nhit < cluster->getNumHits(); nhit++)
  {
    const Storage::Hit* hit = cluster->getHit(nhit);
    const double value = (hit->getValue() > 0) ? hit->getValue() : 1;
    stdevX += pow(value * (hit->getPixX() - cogX), 2.0);
    stdevY += pow(value * (hit->getPixY() - cogY), 2.0);
    weight2 += value*value;
  }

//come mai??? vedere domani!!
  cogX += 0.5;
  cogY += 0.5;

  assert(cluster->getNumHits() > 0 && "ClusterMaker: this is really bad");

  stdevX = sqrt(stdevX / weight2);
  stdevY = sqrt(stdevY / weight2);

  // std::cout << "Number of hits are: " << cluster->getNumHits() << std::endl;
  // std::cout << "Cluster error X: " << stdevX << std::endl;
  // std::cout << "Cluster error Y: " << stdevY << std::endl;
  // std::cout << "" << std::endl;


  //const double errX = (stdevX) ? stdevX : pixErrX;
  //const double errY = (stdevY) ? stdevY : pixErrY;

  cluster->setPosPixel(XYPoint(cogX, cogY));
  cluster->setErrPixel(pixErrX, pixErrY);
}

ClusterMaker::ClusterMaker(unsigned int maxSeparationX, unsigned int maxSeparationY,
                           double maxSeparation) :
  _maxSeparationX(maxSeparationX), _maxSeparationY(maxSeparationY),
  _maxSeparation(maxSeparation)
{
  if (_maxSeparation < 0)
    throw "ClusterMaker: max separation must be positive";
}

std::string ClusterMaker::name() const { return "ClusterMaker"; }

void ClusterMaker::processEvent(uint64_t /* unused */, Storage::Event& event)
{
  for (unsigned int i = 0; i < event.getNumPlanes(); ++i)
    generateClusters(&event, i);
}
void ClusterMaker::finalize() {}

} // namespace Processors
