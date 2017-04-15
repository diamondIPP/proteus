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
  TrackState(float u, float v, float dU = 0, float dV = 0);

  /** Set full covariance matrix from entries.
   *
   * The iterator must point to an array of 10 elements that contain the
   * lower triangular block of the symmetric covariance matrix in compressed
   * row-major layout, i.e. [c00, c10, c11, c20, ...]
   */
  template <typename InputIterator>
  void setCov(InputIterator first);
  void setCovU(float varOffset, float varSlope, float cov = 0);
  void setCovV(float varOffset, float varSlope, float cov = 0);

  void setTrack(Index track);
  void setMatchedCluster(Index cluster);

  /** Covariance matrix of the full parameter vector. */
  const SymMatrix4& cov() const { return m_cov; }
  /** Plane offset in local coordinates. */
  const XYPoint& offset() const { return m_offset; }
  SymMatrix2 covOffset() const { return m_cov.Sub<SymMatrix2>(U, U); }
  /** Slope in local coordinates. */
  const XYVector& slope() const { return m_slope; }
  SymMatrix2 covSlope() const { return m_cov.Sub<SymMatrix2>(Du, Du); }

  Index index() const { return m_index; }
  Index track() const { return m_track; }
  bool isMatched() const { return (m_matchedCluster != kInvalidIndex); }
  Index matchedCluster() const { return m_matchedCluster; }

private:
  enum { U = 0, V = 1, Du = 2, Dv = 3 };

  XYPoint m_offset;
  XYVector m_slope;
  SymMatrix4 m_cov;
  Index m_index;
  Index m_track;
  Index m_matchedCluster;

  friend class SensorEvent;
};

std::ostream& operator<<(std::ostream& os, const TrackState& state);

} // namespace Storage

template <typename InputIterator>
inline void Storage::TrackState::setCov(InputIterator first)
{
  m_cov.SetElements(first, 10, true, true);
}

#endif // PT_TRACKSTATE_H
