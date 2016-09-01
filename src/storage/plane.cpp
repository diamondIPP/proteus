#include "plane.h"

#include <iostream>

#include "cluster.h"
#include "hit.h"

using std::cout;
using std::endl;

Storage::Plane::Plane(unsigned int planeNum)
    : m_planeNum(planeNum)
{
}

void Storage::Plane::addHit(Storage::Hit* hit) { m_hits.push_back(hit); }

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

void Storage::Plane::print(std::ostream& os, const std::string& prefix) const
{
  size_t ihit = 0;
  size_t icluster = 0;

  os << prefix << "hits:\n";
  for (const auto* hit : m_hits) {
    os << prefix << "  hit " << ihit++ << ":\n";
    hit->print(os, prefix + "    ");
  }
  os << prefix << "clusters:\n";
  for (const auto* cluster : m_clusters) {
    os << prefix << "  cluster " << icluster++ << ":\n";
    cluster->print(os, prefix + "    ");
  }
  os.flush();
}
