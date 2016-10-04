#ifndef TRACK_H
#define TRACK_H

#include <iosfwd>
#include <map>
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
  TrackState();
  TrackState(const XYPoint& offset, const XYVector& slope = XYVector(0, 0));
  TrackState(double u, double v, double dU = 0, double dV = 0);

  void setErrOffset(double errU, double errV);
  void setErrU(double errU, double errDu, double cov = 0);
  void setErrV(double errV, double errDv, double cov = 0);

  /** Plane offset in local coordinates. */
  const XYPoint& offset() const { return m_offset; }
  /** Slope in local coordinates. */
  const XYVector& slope() const { return m_slope; }
  const XYVector& errOffset() const { return m_errOffset; }
  const XYVector& errSlope() const { return m_errSlope; }

  /** Full position in the local coordinates. */
  XYZPoint posLocal() const { return XYZPoint(m_offset.x(), m_offset.y(), 0); }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  XYPoint m_offset;
  XYVector m_slope;
  XYVector m_errOffset, m_errSlope;
  double m_covUDu, m_covVDv;

  friend class Track;
};

/** A particle track.
 *
 * The track consist of a set of input clusters and a set of reconstructed
 * track states on selected sensor planes.
 */
class Track {
public:
  typedef std::map<Index, TrackState> TrackStates;

  Track();
  Track(const TrackState& globalState);

  void addLocalState(Index sensor, const TrackState& state);
  void setGlobalState(const TrackState& state) { m_state = state; }
  void setChi2(double chi2) { m_chi2 = chi2; }

  const TrackState& globalState() const { return m_state; }
  const TrackState& localState(Index sensor) const;
  const TrackStates& localStates() const { return m_localStates; }

  /** Adds the cluster to the track but does not inform the cluster about it. */
  void addCluster(Cluster* cluster);
  /** Inform all track clusters that they belong to this track. */
  void fixClusterAssociation();
  Index numClusters() const { return static_cast<Index>(m_clusters.size()); }
  Cluster* getCluster(Index i) { return m_clusters.at(i); }
  const Cluster* getCluster(Index i) const { return m_clusters.at(i); }

  unsigned int getNumClusters() const { return m_clusters.size(); }
  unsigned int getNumMatchedClusters() const
  {
    return m_matchedClusters.size();
  }
  void addMatchedCluster(Cluster* cluster);
  Cluster* getMatchedCluster(unsigned int n) const
  {
    return m_matchedClusters.at(n);
  }
  double getOriginX() const { return m_state.offset().x(); }
  double getOriginY() const { return m_state.offset().y(); }
  double getOriginErrX() const { return m_state.errOffset().x(); }
  double getOriginErrY() const { return m_state.errOffset().y(); }
  double getSlopeX() const { return m_state.slope().x(); }
  double getSlopeY() const { return m_state.slope().y(); }
  double getSlopeErrX() const { return m_state.errSlope().x(); }
  double getSlopeErrY() const { return m_state.errSlope().y(); }
  double getCovarianceX() const { return m_state.m_covUDu; }
  double getCovarianceY() const { return m_state.m_covVDv; }
  double getChi2() const { return m_chi2; }
  int getIndex() const { return m_index; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  TrackState m_state;
  TrackStates m_localStates;
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
