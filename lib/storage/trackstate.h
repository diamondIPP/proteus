/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-12
 */

#ifndef PT_TRACKSTATE_H
#define PT_TRACKSTATE_H

#include <iosfwd>
#include <limits>

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
  /** Construct invalid state; only here for container support. */
  TrackState();
  /** Construct from offset vector, slope vector, and time. */
  TrackState(float u,
             float v,
             float dU,
             float dV,
             float time = std::numeric_limits<float>::quiet_NaN());
  /** Construct from offsets, slopes, and time. */
  TrackState(const Vector2& offset,
             const Vector2& slope,
             float time = std::numeric_limits<float>::quiet_NaN());
  /** Construct from parameter vector and time. */
  template <typename Params>
  TrackState(const Eigen::MatrixBase<Params>& params,
             float time = std::numeric_limits<float>::quiet_NaN())
      : m_params(params)
      , m_cov(SymMatrix4::Zero())
      , m_time(time)
      , m_matchedCluster(kInvalidIndex)
  {
  }

  /** Set the full covariance matrix. */
  template <typename Covariance>
  void setCov(const Eigen::MatrixBase<Covariance>& cov)
  {
    m_cov = cov.template selfadjointView<Eigen::Lower>();
  }
  /** Set only the offset covariance. */
  template <typename Covariance>
  void setCovOffset(const Eigen::MatrixBase<Covariance>& cov)
  {
    m_cov.block<2, 2>(U, U) = cov.template selfadjointView<Eigen::Lower>();
  }
  /** Set only the slope covariance. */
  template <typename Covariance>
  void setCovSlope(const Eigen::MatrixBase<Covariance>& cov)
  {
    m_cov.block<2, 2>(Du, Du) = cov.template selfadjointView<Eigen::Lower>();
  }
  /** Set covariance matrix from packed storage.
   *
   * The iterator must point to a 10 element container that contains the
   * lower triangular block of the symmetric covariance matrix in compressed
   * column-major layout, i.e. [c00, c10, c20, c30, c11, ...].
   */
  template <typename InputIterator>
  void setCovPacked(InputIterator in);
  /** Store the covariance into packed storage.
   *
   * The iterator must point to a 10 element container that will be filled
   * with the lower triangular block of the symmetric covariance matrix in
   * compressed column-major layout, i.e. [c00, c10, c20, c30, c11, ...].
   */
  template <typename OutputIterator>
  void getCovPacked(OutputIterator out) const;

  /** Full parameter vector. */
  auto params() const { return m_params; }
  /** Covariance matrix of the full parameter vector. */
  auto cov() const { return m_cov; }
  /** Track offset in local coordinates. */
  auto offset() const { return m_params.segment<2>(U); }
  auto covOffset() const { return m_cov.block<2, 2>(U, U); }
  /** Track slope in local coordinates. */
  auto slope() const { return m_params.segment<2>(Du); }
  auto covSlope() const { return m_cov.block<2, 2>(Du, Du); }
  /** Track direction in local coordinates. */
  Vector3 direction() const { return {m_params[Du], m_params[Dv], 1}; }
  /** Track time on the local plane. */
  float time() const { return m_time; }

  bool isMatched() const { return (m_matchedCluster != kInvalidIndex); }
  Index matchedCluster() const { return m_matchedCluster; }

private:
  enum { U = 0, V = 1, Du = 2, Dv = 3 };

  Vector4 m_params;
  SymMatrix4 m_cov;
  float m_time;
  Index m_matchedCluster;

  friend class SensorEvent;
  friend class Track;
};

std::ostream& operator<<(std::ostream& os, const TrackState& state);

} // namespace Storage

template <typename InputIterator>
inline void Storage::TrackState::setCovPacked(InputIterator in)
{
  // manual unpacking of compressed, column-major storage
  m_cov(0, 0) = *(in++);
  m_cov(1, 0) = m_cov(0, 1) = *(in++);
  m_cov(2, 0) = m_cov(0, 2) = *(in++);
  m_cov(3, 0) = m_cov(0, 3) = *(in++);
  m_cov(1, 1) = *(in++);
  m_cov(2, 1) = m_cov(1, 2) = *(in++);
  m_cov(3, 1) = m_cov(1, 3) = *(in++);
  m_cov(2, 2) = *(in++);
  m_cov(3, 2) = m_cov(2, 3) = *(in++);
  m_cov(3, 3) = *(in++);
}

template <typename OutputIterator>
inline void Storage::TrackState::getCovPacked(OutputIterator out) const
{
  // manual packing using symmetric, compressed, column-major storage
  *(out++) = m_cov(0, 0);
  *(out++) = m_cov(1, 0);
  *(out++) = m_cov(2, 0);
  *(out++) = m_cov(3, 0);
  *(out++) = m_cov(1, 1);
  *(out++) = m_cov(2, 1);
  *(out++) = m_cov(3, 1);
  *(out++) = m_cov(2, 2);
  *(out++) = m_cov(3, 2);
  *(out++) = m_cov(3, 3);
}

#endif // PT_TRACKSTATE_H
