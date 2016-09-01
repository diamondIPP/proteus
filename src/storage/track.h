#ifndef TRACK_H
#define TRACK_H

#include <iosfwd>
#include <string>
#include <vector>

#include "utils/definitions.h"

namespace Processors {
class TrackMaker;
}

namespace Storage {

class Cluster;

/** Track state, i.e. position and direction, on a local plane.
 *
 * If the local plane is the global xy-plane, the local track description
 * is identical to the usual global description, i.e. global position and
 * slopes along the global z-axis.
 */
class TrackState {
public:
  TrackState(const XYPoint& offset, const XYVector& slope = XYVector(0, 0));
  TrackState(double posU, double posV, double slopeU = 0, double slopeV = 0);

  void setOffsetErr(double errU, double errV);
  void setErrU(double errU, double errDu, double cov = 0);
  void setErrV(double errV, double errDv, double cov = 0);

  /** Plane offset in local coordinates. */
  XYPoint offset() const { return XYPoint(m_u, m_v); }
  XYVector offsetErr() const { return XYVector(m_errU, m_errV); }
  /** Slope in local coordinates. */
  XYVector slope() const { return XYVector(m_du, m_dv); }
  XYVector slopeErr() const { return XYVector(m_errDu, m_errDv); }

  /** Full position in the local coordinates. */
  XYZPoint position() const { return XYZPoint(m_u, m_v, 0); }
  /** Propagated position in the local coordinates. */
  XYZPoint position_at(double w) const
  {
    return XYZPoint(m_u + m_du * w, m_v + m_dv * w, w);
  }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  double m_u, m_v, m_du, m_dv;
  double m_errU, m_errDu, m_covUDu;
  double m_errV, m_errDv, m_covVDv;

  friend class Track;
};

/** A particle track.
 *
 * The track consist of a set of input clusters and a set of reconstructed
 * track states on selected sensor planes.
 */
class Track {
public:
  Track();
  Track(const TrackState& globalState);

  void addCluster(Cluster* cluster);
  void addMatchedCluster(Cluster* cluster);

  Cluster* getCluster(unsigned int n) const { return m_clusters.at(n); }
  Cluster* getMatchedCluster(unsigned int n) const
  {
    return m_matchedClusters.at(n);
  }

  void setGlobalState(const TrackState& state) { m_state = state; }
  void setChi2(double chi2) { m_chi2 = chi2; }

  unsigned int getNumClusters() const { return m_clusters.size(); }
  unsigned int getNumMatchedClusters() const
  {
    return m_matchedClusters.size();
  }
  double getOriginX() const { return m_state.offset().x(); }
  double getOriginY() const { return m_state.offset().y(); }
  double getOriginErrX() const { return m_state.offsetErr().x(); }
  double getOriginErrY() const { return m_state.offsetErr().y(); }
  double getSlopeX() const { return m_state.slope().x(); }
  double getSlopeY() const { return m_state.slope().y(); }
  double getSlopeErrX() const { return m_state.slopeErr().x(); }
  double getSlopeErrY() const { return m_state.slopeErr().y(); }
  double getCovarianceX() const { return m_state.m_covUDu; }
  double getCovarianceY() const { return m_state.m_covVDv; }
  double getChi2() const { return m_chi2; }
  int getIndex() const { return m_index; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  TrackState m_state;
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
