#ifndef PT_SENSOREVENT_H
#define PT_SENSOREVENT_H

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "storage/cluster.h"
#include "storage/hit.h"
#include "storage/trackstate.h"
#include "utils/definitions.h"

namespace Storage {

/** An event for a single sensor containing only local information.
 *
 * Contains hits, clusters, and local track states.
 */
class SensorEvent {
public:
  SensorEvent(Index sensor);

  void clear(uint64_t frame, uint64_t timestamp);

  Index sensor() const { return m_sensor; }
  uint64_t frame() const { return m_frame; }
  uint64_t timestamp() const { return m_timestamp; }

  template <typename... HitParams>
  Hit* addHit(HitParams&&... params);
  Index numHits() const { return static_cast<Index>(m_hits.size()); }
  Hit* getHit(Index i) { return m_hits.at(i).get(); }
  const Hit* getHit(Index i) const { return m_hits.at(i).get(); }

  Cluster* newCluster();
  Index numClusters() const { return static_cast<Index>(m_clusters.size()); }
  Cluster* getCluster(Index i) { return m_clusters.at(i).get(); }
  const Cluster* getCluster(Index i) const { return m_clusters.at(i).get(); }

  void addState(TrackState&& state);
  Index numStates() const { return static_cast<Index>(m_states.size()); }
  TrackState& getState(Index i) { return *m_states[i]; }
  const TrackState& getState(Index i) const { return *m_states[i]; }

  /** Associate one cluster to one track state. */
  void addMatch(Index cluster, Index state);

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  Index m_sensor;
  uint64_t m_frame;
  uint64_t m_timestamp;
  std::vector<std::unique_ptr<Hit>> m_hits;
  std::vector<std::unique_ptr<Cluster>> m_clusters;
  std::vector<std::unique_ptr<TrackState>> m_states;
};

} // namespace Storage

template <typename... HitParams>
inline Storage::Hit* Storage::SensorEvent::addHit(HitParams&&... params)
{
  m_hits.emplace_back(new Hit(std::forward<HitParams>(params)...));
  return m_hits.back().get();
}

inline void Storage::SensorEvent::addState(TrackState&& state)
{
  Index stateId = m_states.size();
  m_states.emplace_back(new TrackState(std::forward<TrackState>(state)));
  m_states.back()->m_index = stateId;
}

#endif // PT_SENSOREVENT_H
