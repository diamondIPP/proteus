/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-12
 */

#ifndef PT_TRACKSTATE_H
#define PT_TRACKSTATE_H

#include <iosfwd>

#include "utils/definitions.h"

namespace Storage {

class Cluster;
class Track;

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

  void setTrack(const Track* track) { m_track = track; }
  const Track* track() const { return m_track; }

  void setMatchedCluster(const Cluster* cluster) { m_matchedCluster = cluster; }
  const Cluster* matchedCluster() const { return m_matchedCluster; }

private:
  enum { U = 0, V = 1, Du = 2, Dv = 3 };

  XYPoint m_offset;
  XYVector m_slope;
  SymMatrix4 m_cov;
  const Track* m_track;
  const Cluster* m_matchedCluster;

  friend class Track;
};

std::ostream& operator<<(std::ostream& os, const TrackState& state);

} // namespace Storage

#endif // PT_TRACKSTATE_H
