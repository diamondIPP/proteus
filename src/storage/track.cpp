#include "track.h"

#include <ostream>

#include "cluster.h"

Storage::Track::Track() : m_state(0, 0, 0, 0), m_redChi2(-1), m_index(-1) {}

Storage::Track::Track(const TrackState& globalState)
    : m_state(globalState), m_redChi2(-1), m_index(-1)
{
}

// NOTE: this doesn't tell the cluster about the track (due to building trial
// tracks)
void Storage::Track::addCluster(Cluster* cluster)
{
  m_clusters.push_back(cluster);
}

void Storage::Track::fixClusterAssociation()
{
  for (auto cluster = m_clusters.begin(); cluster != m_clusters.end();
       ++cluster)
    (*cluster)->setTrack(this);
}

void Storage::Track::setLocalState(Index sensorId, const TrackState& state)
{
  m_localStates[sensorId] = state;
}

bool Storage::Track::hasLocalState(Index sensorId) const
{
  return (0 < m_localStates.count(sensorId));
}

const Storage::TrackState& Storage::Track::getLocalState(Index sensorId) const
{
  return m_localStates.at(sensorId);
}

void Storage::Track::setMatchedCluster(Index sensorId, const Cluster* cluster)
{
  m_matches[sensorId] = cluster;
}

bool Storage::Track::hasMatchedCluster(Index sensorId) const
{
  return (0 < m_matches.count(sensorId));
}

const Storage::Cluster* Storage::Track::getMatchedCluster(Index sensorId) const
{
  return m_matches.at(sensorId);
}

void Storage::Track::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "chi2/ndf: " << m_redChi2 << '\n';
  os << prefix << "points: " << m_clusters.size() << '\n';
  os << prefix << "global: " << m_state << '\n';
  if (!m_localStates.empty()) {
    for (auto is = m_localStates.begin(); is != m_localStates.end(); ++is) {
      const auto& sensorIdId = is->first;
      const auto& state = is->second;
      os << prefix << "  on sensorId " << sensorIdId << ": " << state << '\n';
    }
  }
}
