#include "track.h"

#include <ostream>

#include "cluster.h"

Storage::Track::Track()
    : m_state(0, 0, 0, 0), m_chi2(-1), m_dof(-1), m_index(-1)
{
}

Storage::Track::Track(const TrackState& global)
    : m_state(global), m_chi2(-1), m_dof(-1), m_index(-1)
{
}

// NOTE: this doesn't tell the cluster about the track (due to building trial
// tracks)
void Storage::Track::addCluster(Cluster* cluster)
{
  m_clusters.push_back(cluster);
}

void Storage::Track::freezeClusterAssociation()
{
  for (auto cluster = m_clusters.begin(); cluster != m_clusters.end();
       ++cluster)
    (*cluster)->setTrack(this);
}

void Storage::Track::setMatchedCluster(Index sensorId, const Cluster* cluster)
{
  if (m_matchedClusters.size() <= sensorId) {
    m_matchedClusters.resize(sensorId + 1, NULL);
  }
  m_matchedClusters[sensorId] = cluster;
}

const Storage::Cluster* Storage::Track::getMatchedCluster(Index sensorId) const
{
  if (sensorId < m_matchedClusters.size())
    return m_matchedClusters[sensorId];
  return NULL;
}

void Storage::Track::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "chi2/dof: " << m_chi2 << " / " << m_dof << '\n';
  os << prefix << "points: " << m_clusters.size() << '\n';
  os << prefix << "global: " << m_state << '\n';
}
