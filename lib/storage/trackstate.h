// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-12
 */

#pragma once

#include <iosfwd>
#include <limits>

#include "utils/definitions.h"

namespace proteus {

/** Track state on a plane.
 *
 * If the plane is the global xy-plane, the track description is identical
 * to the usual global description, i.e. global position and slopes along the
 * global z-axis.
 */
class TrackState {
public:
  /** Construct invalid state; only public for container support. */
  TrackState();
  /** Construct from scalar spatial parameters. */
  TrackState(Scalar location0, Scalar location1, Scalar slope0, Scalar slope1);
  /** Construct from position and slope
   *
   * \warning Assumes the position is on the plane and ignores normal component.
   */
  TrackState(const Vector4& position,
             const SymMatrix4& positionCov,
             const Vector2& slope,
             const SymMatrix2& slopeCov);
  /** Construct from full parameter vector. */
  template <typename Params, typename Covariance>
  TrackState(const Eigen::MatrixBase<Params>& params,
             const Eigen::MatrixBase<Covariance>& cov);

  /** Set the full covariance matrix. */
  template <typename Covariance>
  void setCov(const Eigen::MatrixBase<Covariance>& cov);
  /** Set the spatial covariance matrix from packed storage.
   *
   * The iterator must point to a 10 element container that contains the
   * lower triangular block of the symmetric covariance matrix for the
   * parameters [offset0, offset1, slope0, slop1] in compressed column-major
   * layout, i.e.
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
   * parameters [offset0, offset1, slope0, slop1] in compressed column-major
   * layout, i.e.
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
  const Vector6& params() const { return m_params; }
  /** Covariance matrix of the full parameter vector. */
  const SymMatrix6& cov() const { return m_cov; }

  /** On-plane track first spatial dimension. */
  Scalar loc0() const { return m_params[kLoc0]; }
  /** On-plane track second spatial dimension. */
  Scalar loc1() const { return m_params[kLoc1]; }
  /** On-plane spatial track coordinates covariance. */
  auto loc01Cov() const { return m_cov.block<2, 2>(kLoc0, kLoc0); }
  /** Track time. */
  Scalar time() const { return m_params[kTime]; }
  /** Track time variance. */
  Scalar timeVar() const { return m_cov(kTime, kTime); }
  auto onPlane() const { return m_params.segment<3>(kOnPlane); }
  auto onPlaneCov() const { return m_cov.block<3, 3>(kOnPlane, kOnPlane); }
  /** Full track position. */
  Vector4 position() const;
  /** Full track position covariance. */
  SymMatrix4 positionCov() const;

  /** Track slope along the first spatial dimension. */
  Scalar slopeLoc0() const { return m_params[kSlopeLoc0]; }
  /** Track slope along the second spatial dimension. */
  Scalar slopeLoc1() const { return m_params[kSlopeLoc1]; }
  /** Track slope along the temporal dimension (inverse velocity). */
  Scalar slopeTime() const { return m_params[kSlopeTime]; }
  /** Full track tangent in slope parametrization. */
  Vector4 tangent() const;

  Index track() const { return m_track; }

  bool isMatched() const { return (m_matchedCluster != kInvalidIndex); }
  Index matchedCluster() const { return m_matchedCluster; }

private:
  Vector6 m_params;
  SymMatrix6 m_cov;
  Index m_track;
  Index m_matchedCluster;

  friend class SensorEvent;
  friend class Track;

public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

std::ostream& operator<<(std::ostream& os, const TrackState& state);

// inline implementations

inline TrackState::TrackState()
    : m_params(Vector6::Constant(std::numeric_limits<Scalar>::quiet_NaN()))
    , m_cov(SymMatrix6::Constant(std::numeric_limits<Scalar>::quiet_NaN()))
    , m_track(kInvalidIndex)
    , m_matchedCluster(kInvalidIndex)
{
}

template <typename Params, typename Covariance>
inline TrackState::TrackState(const Eigen::MatrixBase<Params>& params,
                              const Eigen::MatrixBase<Covariance>& cov)
    : m_params(params)
    , m_cov(cov.template selfadjointView<Eigen::Lower>())
    , m_track(kInvalidIndex)
    , m_matchedCluster(kInvalidIndex)
{
}

template <typename Covariance>
inline void TrackState::setCov(const Eigen::MatrixBase<Covariance>& cov)
{
  m_cov = cov.template selfadjointView<Eigen::Lower>();
}

template <typename InputIterator>
inline void TrackState::setCovSpatialPacked(InputIterator in)
{
  // manual unpacking of compressed, column-major storage
  // clang-format off
  m_cov(kLoc0,      kLoc0)                                      = *(in++);
  m_cov(kLoc1,      kLoc0)      = m_cov(kLoc0,   kLoc1)         = *(in++);
  m_cov(kSlopeLoc0, kLoc0)      = m_cov(kLoc0,   kSlopeLoc0)    = *(in++);
  m_cov(kSlopeLoc1, kLoc0)      = m_cov(kLoc0,   kSlopeLoc1)    = *(in++);
  m_cov(kLoc1,      kLoc1)                                      = *(in++);
  m_cov(kSlopeLoc0, kLoc1)      = m_cov(kLoc1,   kSlopeLoc0)    = *(in++);
  m_cov(kSlopeLoc1, kLoc1)      = m_cov(kLoc1,   kSlopeLoc1)    = *(in++);
  m_cov(kSlopeLoc0, kSlopeLoc0)                                 = *(in++);
  m_cov(kSlopeLoc1, kSlopeLoc0) = m_cov(kSlopeLoc0, kSlopeLoc1) = *(in++);
  m_cov(kSlopeLoc1, kSlopeLoc1)                                 = *(in++);
  // clang-format on
}

template <typename OutputIterator>
inline void TrackState::getCovSpatialPacked(OutputIterator out) const
{
  // manual packing using symmetric, compressed, column-major storage
  // clang-format off
  *(out++) = m_cov(kLoc0,      kLoc0);
  *(out++) = m_cov(kLoc1,      kLoc0);
  *(out++) = m_cov(kSlopeLoc0, kLoc0);
  *(out++) = m_cov(kSlopeLoc1, kLoc0);
  *(out++) = m_cov(kLoc1,      kLoc1);
  *(out++) = m_cov(kSlopeLoc0, kLoc1);
  *(out++) = m_cov(kSlopeLoc1, kLoc1);
  *(out++) = m_cov(kSlopeLoc0, kSlopeLoc0);
  *(out++) = m_cov(kSlopeLoc1, kSlopeLoc0);
  *(out++) = m_cov(kSlopeLoc1, kSlopeLoc1);
  // clang-format on
}

inline Vector4 TrackState::position() const
{
  Vector4 pos;
  pos[kU] = m_params[kLoc0];
  pos[kV] = m_params[kLoc1];
  pos[kW] = 0;
  pos[kS] = m_params[kTime];
  return pos;
}

inline SymMatrix4 TrackState::positionCov() const
{
  SymMatrix4 cov = SymMatrix4::Zero();
  cov(kU, kU) = m_cov(kLoc0, kLoc0);
  cov(kV, kU) = m_cov(kLoc1, kLoc0);
  cov(kS, kU) = m_cov(kTime, kLoc0);
  cov(kU, kV) = m_cov(kLoc0, kLoc1);
  cov(kV, kV) = m_cov(kLoc1, kLoc1);
  cov(kS, kV) = m_cov(kTime, kLoc1);
  cov(kU, kS) = m_cov(kLoc0, kTime);
  cov(kV, kS) = m_cov(kLoc1, kTime);
  cov(kS, kS) = m_cov(kTime, kTime);
  // measurement is on the plane, i.e. w-components have no uncertainty
  return cov;
}

inline Vector4 TrackState::tangent() const
{
  Vector4 tgt;
  tgt[kU] = m_params[kSlopeLoc0];
  tgt[kV] = m_params[kSlopeLoc1];
  tgt[kW] = 1;
  tgt[kS] = m_params[kSlopeTime];
  return tgt;
}

} // namespace proteus
