#include "sensorevent.h"

#include <ostream>

#include "storage/track.h"

Storage::SensorEvent::SensorEvent(Index sensorId) : m_sensorId(sensorId) {}

void Storage::SensorEvent::clear()
{
  m_hits.clear();
  m_clusters.clear();
  m_states.clear();
}

Storage::Cluster* Storage::SensorEvent::newCluster()
{
  m_clusters.emplace_back(new Cluster(m_sensorId, m_clusters.size() - 1));
  return m_clusters.back().get();
}

void Storage::SensorEvent::print(std::ostream& os,
                                 const std::string& prefix) const
{
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
      if (state.track())
        os << " track=" << state.track()->index();
      os << '\n';
    }
  }
  os.flush();
}
