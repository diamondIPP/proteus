#include "sensorevent.h"

#include <ostream>

#include "storage/track.h"

Storage::SensorEvent::SensorEvent(Index sensor)
    : m_sensor(sensor), m_frame(UINT64_MAX), m_timestamp(UINT64_MAX)
{
}

void Storage::SensorEvent::clear(uint64_t frame, uint64_t timestamp)
{
  m_frame = frame;
  m_timestamp = timestamp;
  m_hits.clear();
  m_clusters.clear();
  m_states.clear();
}

void Storage::SensorEvent::addMatch(Index cluster, Index state)
{
  assert((0 <= cluster) && (cluster < m_clusters.size()) &&
         "invalid cluster index");
  assert((0 <= state) && (state < m_states.size()) && "invalid state index");
  assert(!m_clusters[cluster]->isMatched() &&
         "cluster can only be matched to one track state");
  assert(!m_states[state]->isMatched() &&
         "cluster can only be matched to one track state");

  m_clusters[cluster]->m_matchedState = state;
  m_states[state]->m_matchedCluster = cluster;
}

void Storage::SensorEvent::print(std::ostream& os,
                                 const std::string& prefix) const
{
  os << prefix << "frame: " << m_frame << '\n';
  os << prefix << "timestamp: " << m_timestamp << '\n';
  if (!m_hits.empty()) {
    os << prefix << "hits:\n";
    for (size_t ihit = 0; ihit < m_hits.size(); ++ihit) {
      const Hit& hit = *m_hits[ihit];
      os << prefix << "  hit " << ihit << ": " << hit;
      if (hit.isInCluster())
        os << " cluster=" << hit.cluster();
      os << '\n';
    }
  }
  if (!m_clusters.empty()) {
    os << prefix << "clusters:\n";
    for (size_t icluster = 0; icluster < m_clusters.size(); ++icluster) {
      const Cluster& cluster = *m_clusters[icluster];
      os << prefix << "  cluster " << icluster << ": " << cluster;
      if (cluster.isInTrack())
        os << " track=" << cluster.track();
      os << '\n';
    }
  }
  if (!m_states.empty()) {
    os << prefix << "states:\n";
    for (size_t istate = 0; istate < m_states.size(); ++istate) {
      const TrackState& state = *m_states[istate];
      os << prefix << "  state " << istate << ": " << state;
      if (state.track() != kInvalidIndex)
        os << " track=" << state.track();
      os << '\n';
    }
  }
  os.flush();
}
