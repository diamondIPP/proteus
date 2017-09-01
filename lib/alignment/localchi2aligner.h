#ifndef PT_LOCALCHI2ALIGNER_H
#define PT_LOCALCHI2ALIGNER_H

#include <memory>
#include <utility>
#include <vector>

#include "aligner.h"
#include "mechanics/device.h"
#include "storage/cluster.h"
#include "storage/trackstate.h"
#include "utils/definitions.h"

namespace Alignment {

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
  /**
   * \param precision Expected alignment parameter precision (diagonal elements)
   * \param indices   Only align the parameter subspace defined by these indices
   */
  LocalChi2PlaneFitter(const Vector6& precision, std::vector<size_t> indices);

  /** Add one track-measurement pair to the fitter.
   *
   * \returns true  On sucessful addition of the track
   * \returns false On failure due to non-finite input values
   */
  bool addTrack(const Storage::TrackState& track,
                const Storage::Cluster& measurement,
                const SymMatrix2& weight);
  /** Calculate alignment parameters from all tracks added so far.
   *
   * \returns true  On sucessful minimization
   * \returns false On failure e.g. due to singular normal matrix
   */
  bool minimize(Vector6& a, SymMatrix6& cov) const;

private:
  SymMatrix6 m_wa;
  SymMatrix6 m_fr;
  Vector6 m_y;
  size_t m_numTracks;
  std::vector<size_t> m_subspaceIndices;
};

/** Align sensors using using local chi^2 minimization of track residuals. */
class LocalChi2Aligner : public Aligner {
public:
  /**
   * \param device   The device setup
   * \param alignIds Which sensors should be aligned
   * \param damping  Scale factor for corrections to avoid oscillations
   */
  LocalChi2Aligner(const Mechanics::Device& device,
                   const std::vector<Index>& alignIds,
                   const double damping);
  ~LocalChi2Aligner() = default;

  std::string name() const;
  void execute(const Storage::Event& event);

  Mechanics::Geometry updatedGeometry() const;

private:
  std::vector<std::pair<Index, LocalChi2PlaneFitter>> m_fitters;
  const Mechanics::Device& m_device;
  const double m_damping;
};

} // namespace Alignment

#endif /* end of include guard: PT_LOCALCHI2ALIGNER_H */
