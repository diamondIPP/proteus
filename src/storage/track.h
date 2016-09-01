#ifndef TRACK_H
#define TRACK_H

#include <vector>

namespace Processors {
class TrackMaker;
}

namespace Storage {

class Cluster;

/*******************************************************************************
 * Track class contains all values which define one track, composed of some
 * number of clusters. It also provides a list of the clusters which comprise
 * it.
 */

class Track {
public:
  Track();

  void addCluster(Cluster* cluster);
  void addMatchedCluster(Cluster* cluster);

  Cluster* getCluster(unsigned int n) const { return m_clusters.at(n); }
  Cluster* getMatchedCluster(unsigned int n) const { return m_matchedClusters.at(n); }

  void setOrigin(double x, double y)
  {
    m_originX = x;
    m_originY = y;
  }
  void setOriginErr(double x, double y)
  {
    m_originErrX = x;
    m_originErrY = y;
  }
  void setSlope(double x, double y)
  {
    m_slopeX = x;
    m_slopeY = y;
  }
  void setSlopeErr(double x, double y)
  {
    m_slopeErrX = x;
    m_slopeErrY = y;
  }
  void setChi2(double chi2) { m_chi2 = chi2; }
  void setCovariance(double x, double y)
  {
    m_covarianceX = x;
    m_covarianceY = y;
  }

  unsigned int getNumClusters() const { return m_clusters.size(); }
  unsigned int getNumMatchedClusters() const
  {
    return m_matchedClusters.size();
  }
  double getOriginX() const { return m_originX; }
  double getOriginY() const { return m_originY; }
  double getOriginErrX() const { return m_originErrX; }
  double getOriginErrY() const { return m_originErrY; }
  double getSlopeX() const { return m_slopeX; }
  double getSlopeY() const { return m_slopeY; }
  double getSlopeErrX() const { return m_slopeErrX; }
  double getSlopeErrY() const { return m_slopeErrY; }
  double getCovarianceX() const { return m_covarianceX; }
  double getCovarianceY() const { return m_covarianceY; }
  double getChi2() const { return m_chi2; }
  int getIndex() const { return m_index; }

private:
  double m_originX;
  double m_originY;
  double m_originErrX;
  double m_originErrY;
  double m_slopeX;
  double m_slopeY;
  double m_slopeErrX;
  double m_slopeErrY;
  double m_covarianceX;
  double m_covarianceY;
  double m_chi2;
  int m_index;

  std::vector<Cluster*> m_clusters;
  // NOTE: this isn't stored in file, its a place holder for doing DUT analysis
  std::vector<Cluster*> m_matchedClusters;

  friend class Event;
  friend class Processors::TrackMaker;
};

} // namespace Storage

#endif // TRACK_H
