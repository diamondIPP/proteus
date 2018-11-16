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
  /** Set the spatial covariance matrix from packed storage.
   *
   * The iterator must point to a 10 element container that contains the
   * lower triangular block of the symmetric covariance matrix for the
   * parameters [u, v, u', v'] in compressed column-major layout, i.e.
   *
   *     | c[0]                |
   *     | c[1] c[4]           |
   *     | c[2] c[5] c[7]      |
   *     | c[3] c[6] c[8] c[9] |
   *
   */
  template <typename InputIterator>
  void setCovSpatialPacked(InputIterator in);
  /** Store the spatial covariance into packed storage.
   *
   * The iterator must point to a 10 element container that will be filled
   * with the lower triangular block of the symmetric covariance matrix for the
   * parameters [u, v, u', v'] in compressed column-major layout, i.e.
   *
   *     | c[0]                |
   *     | c[1] c[4]           |
   *     | c[2] c[5] c[7]      |
   *     | c[3] c[6] c[8] c[9] |
   *
   */
  template <typename OutputIterator>
  void getCovSpatialPacked(OutputIterator out) const;

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
inline void Storage::TrackState::setCovSpatialPacked(InputIterator in)
{
  // manual unpacking of compressed, column-major storage
  // clang-format off
  m_cov(U,  U)                  = *(in++);
  m_cov(V,  U)  = m_cov(U,  V)  = *(in++);
  m_cov(Du, U)  = m_cov(U,  Du) = *(in++);
  m_cov(Dv, U)  = m_cov(U,  Dv) = *(in++);
  m_cov(V,  V)                  = *(in++);
  m_cov(Du, V)  = m_cov(V,  Du) = *(in++);
  m_cov(Dv, V)  = m_cov(V,  Du) = *(in++);
  m_cov(Du, Du)                 = *(in++);
  m_cov(Dv, Du) = m_cov(Du, Dv) = *(in++);
  m_cov(Dv, Dv)                 = *(in++);
  // clang-format on
}

template <typename OutputIterator>
inline void Storage::TrackState::getCovSpatialPacked(OutputIterator out) const
{
  // manual packing using symmetric, compressed, column-major storage
  // clang-format off
  *(out++) = m_cov(U,   U);
  *(out++) = m_cov(V,   U);
  *(out++) = m_cov(Du,  U);
  *(out++) = m_cov(Dv,  U);
  *(out++) = m_cov(V,   V);
  *(out++) = m_cov(Du,  V);
  *(out++) = m_cov(Dv,  V);
  *(out++) = m_cov(Du, Du);
  *(out++) = m_cov(Dv, Du);
  *(out++) = m_cov(Dv, Dv);
  // clang-format on
}

#endif // PT_TRACKSTATE_H
