#ifndef PLANE_H
#define PLANE_H

#include <iosfwd>
#include <string>
#include <vector>

#include "utils/definitions.h"

namespace Storage {

class Hit;
class Cluster;

/** An readout event for a single sensor.
 *
 * This provides access to all hits and clusters on a single sensor.
 */
class Plane {
public:
  Index planeNumber() const { return m_planeNum; }

  Index numHits() const { return static_cast<Index>(m_hits.size()); }
  Hit* getHit(Index i) { return m_hits.at(i); }
  const Hit* getHit(Index i) const { return m_hits.at(i); }

  Index numClusters() const { return static_cast<Index>(m_clusters.size()); }
  Cluster* getCluster(Index i) { return m_clusters.at(i); }
  const Cluster* getCluster(Index i) const { return m_clusters.at(i); }

  void addIntercept(double posX, double posY);
  Index numIntercepts() const
  {
    return static_cast<Index>(m_intercepts.size());
  }
  std::pair<double, double> getIntercept(Index i) const
  {
    return m_intercepts.at(i);
  }

  // deprecated accessors
  Index getPlaneNum() const { return m_planeNum; }
  Index getNumHits() const { return numHits(); }
  Index getNumClusters() const { return numClusters(); }
  Index getNumIntercepts() const { return numIntercepts(); }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  Plane(Index planeNum);

  void addHit(Hit* hit);
  void addCluster(Cluster* cluster);

  std::vector<Hit*> m_hits;
  std::vector<Cluster*> m_clusters;
  std::vector<std::pair<double, double>> m_intercepts;
  Index m_planeNum;

  friend class Event;
};

} // namespace Storage

#endif // PLANE_H
