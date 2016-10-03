#include "plane.h"

#include <ostream>

#include "cluster.h"
#include "hit.h"

Storage::Plane::Plane(Index planeNum)
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

void Storage::Plane::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "hits:\n";
  for (size_t ihit = 0; ihit < m_hits.size(); ++ihit)
    os << prefix << "  hit " << ihit << ": " << *m_hits[ihit] << '\n';
  os << prefix << "clusters:\n";
  for (size_t iclu = 0; iclu < m_clusters.size(); ++iclu) {
    os << prefix << "  cluster " << iclu << ":\n";
    m_clusters[iclu]->print(os, prefix + "    ");
  }
  os.flush();
}
