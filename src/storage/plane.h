#ifndef PLANE_H
#define PLANE_H

#include <iosfwd>
#include <string>
#include <vector>

namespace Storage {

class Hit;
class Cluster;

/*******************************************************************************
 * Plane class contains the hits and clusters for one sensor plane as well as
 * a plane number.
 */
class Plane {
public:
  inline unsigned int getPlaneNum() const { return m_planeNum; }
  inline unsigned int getNumHits() const { return m_hits.size(); }
  inline unsigned int getNumClusters() const { return m_clusters.size(); }
  inline unsigned int getNumIntercepts() const { return m_intercepts.size(); }

  Hit* getHit(unsigned int n) const;
  Cluster* getCluster(unsigned int n) const;
  std::pair<double, double> getIntercept(unsigned int n) const;

  void addIntercept(double posX, double posY);

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  Plane(unsigned int planeNum);

  void addHit(Hit* hit);
  void addCluster(Cluster* cluster);

  unsigned int m_planeNum;
  std::vector<Hit*> m_hits;
  std::vector<Cluster*> m_clusters;
  std::vector<std::pair<double, double>> m_intercepts;

  friend class Event;
};

} // namespace Storage

#endif // PLANE_H
