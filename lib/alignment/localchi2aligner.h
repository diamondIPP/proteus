// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "aligner.h"
#include "utils/definitions.h"

namespace proteus {

class Cluster;
class Device;
class TrackState;

/** Fit alignment parameters using a chi^2 minimization of track residuals.
 *
 * This is an implementation of
 *
 *     V. Karimaeki et al., 2003, arXiv:physics/0306034
 *
 * that uses a chi^2 expression in the local coordinate system and a straight
 * track assumption to find optimal alignment parameters that minimize the track
 * residuals.
 *
 * Alignment parameters are [du, dv, dw, dalpha, dbeta, dgamma].
 */
class LocalChi2PlaneFitter {
public:
  /** Construct an zeroed fitter.
   *
   * \param scaling  Internal parameter scaling e.g. for numerical stability.
   */
  LocalChi2PlaneFitter(const DiagMatrix6& scaling);

  /** Add one track-measurement pair to the fitter.
   *
   * \returns true  On successful addition of the track
   * \returns false On failure due to non-finite input values
   */
  bool addTrack(const TrackState& track,
                const Cluster& measurement,
                const SymMatrix2& weight);
  /** Calculate alignment parameters from all tracks added so far.
   *
   * \returns true  On successful minimization
   * \returns false On minimization failure e.g. due to singularities
   */
  bool minimize(Vector6& a, SymMatrix6& cov) const;

private:
  DiagMatrix6 m_scaling;
  SymMatrix6 m_fr;
  Vector6 m_y;
  size_t m_numTracks;
};

/** Align sensors using using local chi^2 minimization of track residuals. */
class LocalChi2Aligner : public Aligner {
public:
  /**
   * \param device   The device setup
   * \param alignIds Which sensors should be aligned
   * \param damping  Scale factor for corrections to avoid oscillations
   */
  LocalChi2Aligner(const Device& device,
                   const std::vector<Index>& alignIds,
                   const double damping);
  ~LocalChi2Aligner() = default;

  std::string name() const;
  void execute(const Event& event);

  Geometry updatedGeometry() const;

private:
  std::vector<std::pair<Index, LocalChi2PlaneFitter>> m_fitters;
  const Device& m_device;
  const double m_damping;
};

} // namespace proteus
