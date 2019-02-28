#include "sensorevent.h"

#include <algorithm>
#include <ostream>

#include "storage/track.h"

Storage::SensorEvent::SensorEvent()
    : m_frame(UINT64_MAX), m_timestamp(UINT64_MAX)
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

bool Storage::SensorEvent::hasLocalState(Index itrack) const
{
  return (std::find_if(m_states.begin(), m_states.end(),
                       [=](const TrackState& state) {
                         return (state.track() == itrack);
                       }) != m_states.end());
}

const Storage::TrackState&
Storage::SensorEvent::getLocalState(Index itrack) const
{
  auto it = std::find_if(
      m_states.begin(), m_states.end(),
      [=](const TrackState& state) { return (state.track() == itrack); });
  if (it == m_states.end()) {
    throw std::out_of_range("Invalid track index");
  }
  return *it;
}

void Storage::SensorEvent::addMatch(Index icluster, Index itrack)
{
  auto isInTrack = [=](const TrackState& state) {
    return (state.track() == itrack);
  };
  auto& cluster = m_clusters.at(icluster);
  auto state = std::find_if(m_states.begin(), m_states.end(), isInTrack);
  if (state == m_states.end()) {
    throw std::out_of_range("Invalid track index");
  }

  // remove previous associations
  if (cluster->isMatched()) {
    auto other = std::find_if(m_states.begin(), m_states.end(), isInTrack);
    if (other != m_states.end()) {
      other->m_matchedCluster = kInvalidIndex;
    }
  }
  if (state->isMatched()) {
    m_clusters.at(state->matchedCluster())->m_matchedState = kInvalidIndex;
  }

  // set new association
  cluster->m_matchedState = itrack;
  state->m_matchedCluster = icluster;
}

void Storage::SensorEvent::print(std::ostream& os,
                                 const std::string& prefix) const
{
  os << prefix << "frame: " << m_frame << '\n';
  os << prefix << "timestamp: " << m_timestamp << '\n';
  if (!m_hits.empty()) {
    os << prefix << "hits:\n";
    for (size_t ihit = 0; ihit < m_hits.size(); ++ihit)
      os << prefix << "  " << ihit << ": " << *m_hits[ihit] << '\n';
  }
  if (!m_clusters.empty()) {
    os << prefix << "clusters:\n";
    for (size_t icluster = 0; icluster < m_clusters.size(); ++icluster) {
      os << prefix << "  " << icluster << ": " << *m_clusters[icluster] << '\n';
    }
  }
  if (!m_states.empty()) {
    os << prefix << "track states:\n";
    for (const auto& ts : m_states)
      os << prefix << "  " << ts.track() << ": " << ts << '\n';
  }
  os.flush();
}
