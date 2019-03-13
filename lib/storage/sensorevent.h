#ifndef PT_SENSOREVENT_H
#define PT_SENSOREVENT_H

#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "storage/cluster.h"
#include "storage/hit.h"
#include "storage/trackstate.h"
#include "utils/definitions.h"

namespace Processors {
class BinaryClusterizer;
class ValueWeightedClusterizer;
class FastestHitClusterizer;
} // namespace Processors
namespace Storage {

/** An event for a single sensor containing only local information.
 *
 * Contains hits, clusters, and local track states.
 */
class SensorEvent {
public:
  using TrackStates = std::map<Index, TrackState>;

  SensorEvent();

  void clear(uint64_t frame, uint64_t timestamp);

  uint64_t frame() const { return m_frame; }
  uint64_t timestamp() const { return m_timestamp; }

  template <typename... Params>
  Hit& addHit(Params&&... params);
  Index numHits() const { return static_cast<Index>(m_hits.size()); }
  Hit& getHit(Index i) { return *m_hits.at(i); }
  const Hit& getHit(Index i) const { return *m_hits.at(i); }

  template <typename... Params>
  Cluster& addCluster(Params&&... params);
  Index numClusters() const { return static_cast<Index>(m_clusters.size()); }
  Cluster& getCluster(Index i) { return *m_clusters.at(i); }
  const Cluster& getCluster(Index i) const { return *m_clusters.at(i); }

  /** Set a local track state for the given track. */
  template <typename... Params>
  void setLocalState(Index track, Params&&... params);
  const TrackState& getLocalState(Index i) const { return m_states.at(i); }
  const TrackStates& localStates() const { return m_states; }

  /** Associate one cluster to one track state. */
  void addMatch(Index cluster, Index track);

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  uint64_t m_frame;
  uint64_t m_timestamp;
  std::vector<std::unique_ptr<Hit>> m_hits;
  std::vector<std::unique_ptr<Cluster>> m_clusters;
  TrackStates m_states;

  friend class Event;
  friend class Processors::BinaryClusterizer;
  friend class Processors::ValueWeightedClusterizer;
  friend class Processors::FastestHitClusterizer;
};

} // namespace Storage

template <typename... HitParams>
inline Storage::Hit& Storage::SensorEvent::addHit(HitParams&&... params)
{
  m_hits.emplace_back(new Hit(std::forward<HitParams>(params)...));
  return *m_hits.back();
}

template <typename... Params>
inline Storage::Cluster& Storage::SensorEvent::addCluster(Params&&... params)
{
  m_clusters.emplace_back(new Cluster(std::forward<Params>(params)...));
  m_clusters.back()->m_index = m_clusters.size() - 1;
  return *m_clusters.back();
}

template <typename... Params>
inline void Storage::SensorEvent::setLocalState(Index track, Params&&... params)
{
  m_states[track] = TrackState(std::forward<Params>(params)...);
}

#endif // PT_SENSOREVENT_H
