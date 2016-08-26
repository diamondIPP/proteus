#include "plane.h"

#include <cassert>
#include <iostream>
#include <vector>

#include "cluster.h"
#include "hit.h"

using std::cout;
using std::endl;

Storage::Plane::Plane(unsigned int planeNum)
    : _planeNum(planeNum)
    , _numHits(0)
    , _numClusters(0)
    , _numIntercepts(0)
{
}

void Storage::Plane::addHit(Storage::Hit* hit)
{
  _hits.push_back(hit);
  _numHits++;
}

void Storage::Plane::addCluster(Storage::Cluster* cluster)
{
  _clusters.push_back(cluster);
  cluster->m_plane = this;
  _numClusters++;
}

void Storage::Plane::addIntercept(double posX, double posY)
{
  _intercepts.push_back(std::pair<double, double>(posX, posY));
  _numIntercepts++;
}

Storage::Hit* Storage::Plane::getHit(unsigned int n) const
{
  assert(n < getNumHits() && "Plane: hit index exceeds vector size");
  return _hits.at(n);
}

Storage::Cluster* Storage::Plane::getCluster(unsigned int n) const
{
  assert(n < getNumClusters() && "Plane: cluster index exceeds vector size");
  return _clusters.at(n);
}

std::pair<double, double> Storage::Plane::getIntercept(unsigned int n) const
{
  assert(n < getNumIntercepts() &&
         "Plane: intercept index exceeds vector size");
  return _intercepts.at(n);
}

void Storage::Plane::print() const
{
  cout << "PLANE:\n"
       << " - Address: " << this << "\n"
       << " - Num clusters: " << getNumClusters() << "\n"
       << " - Num hits: " << getNumHits() << endl;

  for (unsigned int ncluster = 0; ncluster < getNumClusters(); ncluster++)
    cout << "\n    ** CLUSTER # " << ncluster << endl
         << getCluster(ncluster)->printStr(6) << endl;

  for (unsigned int nhit = 0; nhit < getNumHits(); nhit++) {
    cout << "\n    ** HIT # " << nhit << endl;
    getHit(nhit)->print(cout, "      ");
  }
}
