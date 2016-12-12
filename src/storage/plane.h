#ifndef PT_PLANE_H
#define PT_PLANE_H

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "storage/cluster.h"
#include "storage/hit.h"
#include "storage/trackstate.h"
#include "utils/definitions.h"

namespace Storage {

/** An readout event for a single sensor.
 *
 * This provides access to all hits and clusters on a single sensor.
 */
class Plane {
public:
  Index sensorId() const { return m_sensorId; }

  Hit* newHit();
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

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  Plane(Index sensorId);

  void clear();

  std::vector<std::unique_ptr<Hit>> m_hits;
  std::vector<std::unique_ptr<Cluster>> m_clusters;
  std::vector<std::unique_ptr<TrackState>> m_states;
  Index m_sensorId;

  friend class Event;
};

} // namespace Storage

inline void Storage::Plane::addState(TrackState&& state)
{
  m_states.emplace_back(new TrackState(std::forward<TrackState>(state)));
}

#endif // PT_PLANE_H
