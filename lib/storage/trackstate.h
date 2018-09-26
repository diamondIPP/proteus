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
  /** Construct a state w/ undefined parameters. */
  TrackState();
  /** Construct from local offset and direction vector. */
  TrackState(const Vector2& offset, const Vector3& direction);
  /** Construct from local offset and slope values. */
  TrackState(float u, float v, float dU = 0, float dV = 0);

  /** Set the full covariance matrix. */
  void setCov(const SymMatrix4& cov) { m_cov = cov; }
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
  /** Set only the offset covariance. */
  void setCovOffset(const SymMatrix2& covOffset);

  /** Full parameter vector. */
  const Vector4& params() const { return m_params; }
  /** Covariance matrix of the full parameter vector. */
  const SymMatrix4& cov() const { return m_cov; }
  /** Plane offset in local coordinates. */
  Vector2 offset() const { return m_params.Sub<Vector2>(U); }
  SymMatrix2 covOffset() const { return m_cov.Sub<SymMatrix2>(U, U); }
  /** Slope in local coordinates. */
  Vector2 slope() const { return m_params.Sub<Vector2>(Du); }
  SymMatrix2 covSlope() const { return m_cov.Sub<SymMatrix2>(Du, Du); }
  /** Direction vector in local coordinates. */
  Vector3 direction() const { return {m_params[Du], m_params[Dv], 1}; }

  bool isMatched() const { return (m_matchedCluster != kInvalidIndex); }
  Index matchedCluster() const { return m_matchedCluster; }

private:
  enum { U = 0, V = 1, Du = 2, Dv = 3 };

  Vector4 m_params;
  SymMatrix4 m_cov;
  Index m_matchedCluster;

  friend class SensorEvent;
  friend class Track;
};

std::ostream& operator<<(std::ostream& os, const TrackState& state);

} // namespace Storage

template <typename InputIterator>
inline void Storage::TrackState::setCov(InputIterator first)
{
  m_cov.SetElements(first, 10, true, true);
}

#endif // PT_TRACKSTATE_H
