#ifndef PT_TRACK_H
#define PT_TRACK_H

#include <cmath>
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

  void setCovOffset(const SymMatrix2& cov) { m_cov.Place_at(cov, U, U); };
  void setCovU(double varOffset, double varSlope, double cov = 0);
  void setCovV(double varOffset, double varSlope, double cov = 0);
  void setErrU(double stdOffset, double stdSlope, double cov = 0);
  void setErrV(double stdOffset, double stdSlope, double cov = 0);

  /** Plane offset in local coordinates. */
  const XYPoint& offset() const { return m_offset; }
  SymMatrix2 covOffset() const { return m_cov.Sub<SymMatrix2>(U, U); }
  double stdOffsetU() const { return std::sqrt(m_cov(U, U)); }
  double stdOffsetV() const { return std::sqrt(m_cov(V, V)); }
  /** Slope in local coordinates. */
  const XYVector& slope() const { return m_slope; }
  SymMatrix2 covSlope() const { return m_cov.Sub<SymMatrix2>(Du, Du); }
  double stdSlopeU() const { return std::sqrt(m_cov(Du, Du)); }
  double stdSlopeV() const { return std::sqrt(m_cov(Dv, Dv)); }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  enum { U = 0, V = 1, Du = 2, Dv = 3 };

  XYPoint m_offset;
  XYVector m_slope;
  SymMatrix4 m_cov;

  friend class Track;
};

/** A particle track.
 *
 * The track consist of a set of input clusters and a set of reconstructed
 * track states on selected sensorId planes.
 */
class Track {
public:
  Track();
  Track(const TrackState& globalState);

  /** Adds the cluster to the track but does not inform the cluster about it. */
  void addCluster(Cluster* cluster);
  /** Inform all track clusters that they belong to this track now. */
  void fixClusterAssociation();
  Index numClusters() const { return static_cast<Index>(m_clusters.size()); }
  Cluster* getCluster(Index i) { return m_clusters.at(i); }
  const Cluster* getCluster(Index i) const { return m_clusters.at(i); }

  // global track information.
  void setGlobalState(const TrackState& state) { m_state = state; }
  void setGoodnessOfFit(double chi2, int ndf) { m_redChi2 = chi2 / ndf; }
  void setGoodnessOfFit(double reducedChi2) { m_redChi2 = reducedChi2; }
  const TrackState& globalState() const { return m_state; }
  double reducedChi2() const { return m_redChi2; }

  // local state information.
  void setLocalState(Index sensorId, const TrackState& state);
  bool hasLocalState(Index sensorId) const;
  const TrackState& getLocalState(Index sensorId) const;

  // matched cluster information. only used transiently, not stored.
  void setMatchedCluster(Index sensorId, const Cluster* cluster);
  bool hasMatchedCluster(Index sensorId) const;
  const Cluster* getMatchedCluster(Index sensorId) const;

  unsigned int getNumClusters() const { return m_clusters.size(); }
  double getOriginX() const { return m_state.offset().x(); }
  double getOriginY() const { return m_state.offset().y(); }
  double getOriginErrX() const { return m_state.stdOffsetU(); }
  double getOriginErrY() const { return m_state.stdOffsetV(); }
  double getSlopeX() const { return m_state.slope().x(); }
  double getSlopeY() const { return m_state.slope().y(); }
  double getSlopeErrX() const { return m_state.stdSlopeU(); }
  double getSlopeErrY() const { return m_state.stdSlopeV(); }
  double getCovarianceX() const { return m_state.m_cov(0, 2); }
  double getCovarianceY() const { return m_state.m_cov(1, 3); }
  double getChi2() const { return m_redChi2; }
  int getIndex() const { return m_index; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  TrackState m_state;
  std::map<Index, TrackState> m_localStates;
  double m_redChi2;
  int m_index;

  std::vector<Cluster*> m_clusters;
  // NOTE: this isn't stored in file, its a place holder for doing DUT analysis
  std::vector<Cluster*> m_matchedClusters;

  std::map<Index, const Cluster*> m_matches;

  friend class Event;
  friend class Processors::TrackMaker;
};

std::ostream& operator<<(std::ostream& os, const TrackState& state);

} // namespace Storage

#endif // PT_TRACK_H
