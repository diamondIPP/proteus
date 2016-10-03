#ifndef PLANE_H
#define PLANE_H

#include <iosfwd>
#include <string>
#include <vector>

#include "utils/definitions.h"

namespace Storage {

class Hit;
class Cluster;

/*******************************************************************************
 * Plane class contains the hits and clusters for one sensor plane as well as
 * a plane number.
 */
class Plane {
public:
  Index getPlaneNum() const { return m_planeNum; }
  Index getNumHits() const { return static_cast<Index>(m_hits.size()); }
  Index getNumClusters() const { return static_cast<Index>(m_clusters.size()); }
  Index getNumIntercepts() const
  {
    return static_cast<Index>(m_intercepts.size());
  }

  Hit* getHit(Index i) const;
  Cluster* getCluster(Index i) const;
  std::pair<double, double> getIntercept(Index i) const;

  void addIntercept(double posX, double posY);

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  Plane(Index planeNum);

  void addHit(Hit* hit);
  void addCluster(Cluster* cluster);

  Index m_planeNum;
  std::vector<Hit*> m_hits;
  std::vector<Cluster*> m_clusters;
  std::vector<std::pair<double, double>> m_intercepts;

  friend class Event;
};

} // namespace Storage

#endif // PLANE_H
