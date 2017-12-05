#include "track.h"

#include <ostream>

#include "cluster.h"

Storage::Track::Track() : m_chi2(-1), m_dof(-1), m_index(-1) {}

Storage::Track::Track(const TrackState& global)
    : m_state(global), m_chi2(-1), m_dof(-1), m_index(-1)
{
}

// NOTE: this doesn't tell the cluster about the track
void Storage::Track::addCluster(Index sensor, Cluster& cluster)
{
  m_clusters.emplace(sensor, cluster);
}

void Storage::Track::freezeClusterAssociation()
{
  for (const auto& c : m_clusters)
    c.second.get().setTrack(m_index);
}

void Storage::Track::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "chi2/dof: " << m_chi2 << " / " << m_dof << '\n';
  os << prefix << "size: " << m_clusters.size() << '\n';
  os << prefix << "global: " << m_state << '\n';
}