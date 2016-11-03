#ifndef PLANE_H
#define PLANE_H

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "storage/cluster.h"
#include "storage/hit.h"
#include "utils/definitions.h"

namespace Storage {

/** An readout event for a single sensor.
 *
 * This provides access to all hits and clusters on a single sensor.
 */
class Plane {
public:
  Index sensorId() const { return m_sensorId; }

  Storage::Hit* newHit();
  Index numHits() const { return static_cast<Index>(m_hits.size()); }
  Hit* getHit(Index i) { return m_hits.at(i).get(); }
  const Hit* getHit(Index i) const { return m_hits.at(i).get(); }

  Storage::Cluster* newCluster();
  Index numClusters() const { return static_cast<Index>(m_clusters.size()); }
  Cluster* getCluster(Index i) { return m_clusters.at(i).get(); }
  const Cluster* getCluster(Index i) const { return m_clusters.at(i).get(); }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  Plane(Index sensorId);

  void clear();

  std::vector<std::unique_ptr<Hit>> m_hits;
  std::vector<std::unique_ptr<Cluster>> m_clusters;
  Index m_sensorId;

  friend class Event;
};

} // namespace Storage

#endif // PLANE_H
