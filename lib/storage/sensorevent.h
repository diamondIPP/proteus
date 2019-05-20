// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#pragma once

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "storage/cluster.h"
#include "storage/hit.h"
#include "storage/trackstate.h"
#include "utils/definitions.h"

namespace proteus {

class BinaryClusterizer;
class ValueWeightedClusterizer;
class FastestHitClusterizer;

/** An event for a single sensor containing only local information.
 *
 * Contains hits, clusters, and local track states.
 */
class SensorEvent {
public:
  SensorEvent();

  void clear(uint64_t frame, uint64_t timestamp);

  uint64_t frame() const { return m_frame; }
  uint64_t timestamp() const { return m_timestamp; }

  template <typename... Params>
  Hit& addHit(Params&&... params);
  Index numHits() const { return static_cast<Index>(m_hits.size()); }
  Hit& getHit(Index ihit) { return *m_hits.at(ihit); }
  const Hit& getHit(Index ihit) const { return *m_hits.at(ihit); }

  template <typename... Params>
  Cluster& addCluster(Params&&... params);
  Index numClusters() const { return static_cast<Index>(m_clusters.size()); }
  Cluster& getCluster(Index icluster) { return *m_clusters.at(icluster); }
  const Cluster& getCluster(Index icluster) const
  {
    return *m_clusters.at(icluster);
  }

  /** Set a local track state for the given track. */
  template <typename... Params>
  void setLocalState(Index itrack, Params&&... params);
  /** Check if a local state is available for a specific track. */
  bool hasLocalState(Index itrack) const;
  const TrackState& getLocalState(Index itrack) const;
  const std::vector<TrackState>& localStates() const { return m_states; }

  /** Associate one cluster to one track state.
   *
   * Any previously existing association for either the cluster or the track
   * will be overwritten.
   */
  void addMatch(Index icluster, Index itrack);

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  uint64_t m_frame;
  uint64_t m_timestamp;
  std::vector<std::unique_ptr<Hit>> m_hits;
  std::vector<std::unique_ptr<Cluster>> m_clusters;
  std::vector<TrackState> m_states;

  friend class Event;
  friend class BinaryClusterizer;
  friend class ValueWeightedClusterizer;
  friend class FastestHitClusterizer;
};

// inline implementations

template <typename... HitParams>
inline Hit& SensorEvent::addHit(HitParams&&... params)
{
  m_hits.emplace_back(
      std::make_unique<Hit>(std::forward<HitParams>(params)...));
  return *m_hits.back();
}

template <typename... Params>
inline Cluster& SensorEvent::addCluster(Params&&... params)
{
  m_clusters.emplace_back(
      std::make_unique<Cluster>(std::forward<Params>(params)...));
  m_clusters.back()->m_index = m_clusters.size() - 1;
  return *m_clusters.back();
}

template <typename... Params>
inline void SensorEvent::setLocalState(Index itrack, Params&&... params)
{
  auto it = std::find_if(
      m_states.begin(), m_states.end(),
      [=](const TrackState& state) { return (state.track() == itrack); });
  if (it != m_states.end()) {
    *it = TrackState(std::forward<Params>(params)...);
    it->m_track = itrack;
  } else {
    m_states.emplace_back(std::forward<Params>(params)...);
    m_states.back().m_track = itrack;
  }
}

} // namespace proteus
