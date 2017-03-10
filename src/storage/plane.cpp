#include "plane.h"

#include <ostream>

#include "storage/track.h"

void Storage::Plane::clear()
{
  m_states.clear();
  m_clusters.clear();
  m_hits.clear();
}

Storage::Plane::Plane(Index sensorId) : m_sensorId(sensorId) {}

Storage::Cluster* Storage::Plane::newCluster()
{
  m_clusters.emplace_back(new Cluster());
  m_clusters.back()->m_index = m_clusters.size() - 1;
  m_clusters.back()->m_plane = this;
  return m_clusters.back().get();
}

void Storage::Plane::print(std::ostream& os, const std::string& prefix) const
{
  if (!m_hits.empty()) {
    os << prefix << "hits:\n";
    for (size_t ihit = 0; ihit < m_hits.size(); ++ihit) {
      const Hit& hit = *m_hits[ihit];
      os << prefix << "  hit " << ihit << ": " << hit;
      if (hit.cluster())
        os << " cluster=" << hit.cluster()->index();
      os << '\n';
    }
  }
  if (!m_clusters.empty()) {
    os << prefix << "clusters:\n";
    for (size_t icluster = 0; icluster < m_clusters.size(); ++icluster) {
      const Cluster& cluster = *m_clusters[icluster];
      os << prefix << "  cluster " << icluster << ": " << cluster;
      if (cluster.track())
        os << " track=" << cluster.track()->index();
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
