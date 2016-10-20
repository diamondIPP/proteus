#include "plane.h"

#include <ostream>

void Storage::Plane::clear()
{
  m_intercepts.clear();
  m_clusters.clear();
  m_hits.clear();
}

Storage::Plane::Plane(Index planeNum)
    : m_planeNum(planeNum)
{
}

Storage::Hit* Storage::Plane::newHit()
{
  m_hits.emplace_back(new Hit());
  return m_hits.back().get();
}

Storage::Cluster* Storage::Plane::newCluster()
{
  m_clusters.emplace_back(new Cluster());
  m_clusters.back()->m_index = m_clusters.size() - 1;
  m_clusters.back()->m_plane = this;
  return m_clusters.back().get();
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
