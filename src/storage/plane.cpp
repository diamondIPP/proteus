#include "plane.h"

#include <iostream>
#include <vector>

#include "cluster.h"
#include "hit.h"

using std::cout;
using std::endl;

Storage::Plane::Plane(unsigned int planeNum)
    : m_planeNum(planeNum)
{
}

void Storage::Plane::addHit(Storage::Hit* hit)
{
  m_hits.push_back(hit);
}

void Storage::Plane::addCluster(Storage::Cluster* cluster)
{
  m_clusters.push_back(cluster);
  cluster->m_plane = this;
}

void Storage::Plane::addIntercept(double posX, double posY)
{
  m_intercepts.push_back(std::pair<double, double>(posX, posY));
}

Storage::Hit* Storage::Plane::getHit(unsigned int n) const
{
  return m_hits.at(n);
}

Storage::Cluster* Storage::Plane::getCluster(unsigned int n) const
{
  return m_clusters.at(n);
}

std::pair<double, double> Storage::Plane::getIntercept(unsigned int n) const
{
  return m_intercepts.at(n);
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
