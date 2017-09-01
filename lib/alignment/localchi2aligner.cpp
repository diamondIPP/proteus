#include "localchi2aligner.h"

#include <algorithm>
#include <cmath>

#include "storage/event.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(LocalChi2Aligner)

// local helper functions

// Solve f * x = y using only a subspace of the solution space.
//
// Entries of the solution vector x that are not in the selected subspace, i.e.
// whose index is not in indices, are zeroed.
template <typename T, unsigned int kFull, size_t kSub>
static bool solveSubspace(const SymMatrix<T, kFull>& f,
                          const Vector<T, kFull>& y,
                          const std::array<size_t, kSub>& indices,
                          SymMatrix<T, kFull>& finv,
                          Vector<T, kFull>& x)
{
  static_assert(0 < kFull, "Input space must have at least one dimension.");
  static_assert(kSub <= kFull, "Subspace must be contained in input space.");

  // project the selected subspace
  SymMatrix<double, kSub> fsub;
  Vector<double, kSub> ysub;
  for (unsigned int i = 0; i < kSub; ++i) {
    ysub[i] = y[indices[i]];
    for (unsigned int j = 0; j < kSub; ++j) {
      fsub(i, j) = f(indices[i], indices[j]);
    }
  }
  DEBUG("subspace normal vector:\n", ysub);
  DEBUG("subspace normal matrix:\n", fsub);

  // solve normal equations only in the selected subspace
  int ifail = 0;
  auto fsub_inv = fsub.InverseChol(ifail);
  if (ifail) {
    return false;
  }

  // solution in the selected subspace
  Vector<double, kSub> xsub = fsub_inv * ysub;
  DEBUG("subspace inverted matrix:\n", fsub_inv);

  // expand the subspace parameters back to the full space; leave rest zero
  x *= 0.0;
  finv *= 0.0;
  for (unsigned int i = 0; i < kSub; ++i) {
    x[indices[i]] = xsub[i];
    for (unsigned int j = 0; j < kSub; ++j) {
      finv(indices[i], indices[j]) = fsub_inv(i, j);
    }
  }

  return true;
}

template <size_t kSize, typename T>
static std::array<T, kSize> makeArray(const std::vector<T>& x)
{
  std::array<T, kSize> ret;
  for (size_t i = 0; i < x.size(); ++i) {
    ret[i] = x[i];
  }
  return ret;
}

// Solve f * x = y using only a dynamic subspace of the solution space.
//
// Same as above but the dimension of the subspace is only known at runtime.
// Supports only dimensions 1-6.
template <typename T, unsigned int kFull>
static bool solveSubspace(const SymMatrix<T, kFull>& f,
                          const Vector<T, kFull>& y,
                          const std::vector<size_t>& indices,
                          SymMatrix<T, kFull>& finv,
                          Vector<T, kFull>& x)
{
  assert((indices.size() <= kFull) &&
         "Subspace must be contained in input space.");

  // TODO msmk 2018-10-02
  // this is a hack because SMatrix can not match fixed size and dynamic
  // vectors. Should be replaced once we switch to a sane linear algebra
  // library.
  switch (indices.size()) {
  case 1:
    return solveSubspace(f, y, makeArray<1>(indices), finv, x);
  case 2:
    return solveSubspace(f, y, makeArray<2>(indices), finv, x);
  case 3:
    return solveSubspace(f, y, makeArray<3>(indices), finv, x);
  case 4:
    return solveSubspace(f, y, makeArray<4>(indices), finv, x);
  case 5:
    return solveSubspace(f, y, makeArray<5>(indices), finv, x);
  case 6:
    return solveSubspace(f, y, makeArray<6>(indices), finv, x);
  default:
    return false;
  }
}

// Map [du, dv, dw, dalpha, dbeta, dgamma] to track offset changes.
//
// Assumes the track will stay the constant in the global frame and the
// alignment corrections result in a different intersection point.
//
// This Jacobian is equivalent to eq. 17 from V. Karimaeki et al., 2003,
// arXiv:physics/0306034 w/ some sign modifications to adjust for
// a different alignment parameter convention (see Geometry::Plane).
static Matrix26 jacobianOffsetAlignment(const Storage::TrackState& state)
{
  auto offsetU = state.offset()[0];
  auto offsetV = state.offset()[1];
  auto slopeU = state.slope()[0];
  auto slopeV = state.slope()[1];

  Matrix26 jac;
  jac(0, 0) = -1;                // d u / d du
  jac(0, 1) = 0;                 // d u / d dv
  jac(0, 2) = slopeU;            // d u / d dw
  jac(0, 3) = slopeU * offsetV;  // d u / d dalpha
  jac(0, 4) = -slopeU * offsetU; // d u / d dbeta
  jac(0, 5) = offsetV;           // d u / d dgamma
  jac(1, 0) = 0;                 // d v / d du
  jac(1, 1) = -1;                // d v / d dv
  jac(1, 2) = slopeV;            // d v / d dw
  jac(1, 3) = slopeV * offsetV;  // d v / d dalpha
  jac(1, 4) = -slopeV * offsetU; // d v / d dbeta
  jac(1, 5) = -offsetU;          // d v / d dgamma
  return jac;
}

// Derive expected precision for alignment parameters from sensor geometry.
//
// The values correspond to the smallest value that can be sensibly resolved
// due to limits on the sensor size, pitch, and beam parameters.
static Vector6 estimateAlignmentPrecision(const Mechanics::Sensor& sensor)
{
  auto rsqrt = [](double x) { return 1.0 / std::sqrt(x); };
  auto square = [](double x) { return x * x; };

  // use pixel pitch as proxy for resolution. this should be fine as long
  // as the additional tracking uncertainty is small compared to the pitch.
  auto varU = square(sensor.pitchCol()) / 12.0;
  auto varV = square(sensor.pitchRow()) / 12.0;
  auto l2U = square(sensor.sensitiveAreaLocal().length(0));
  auto l2V = square(sensor.sensitiveAreaLocal().length(1));
  auto s2U = square(sensor.beamSlope()[0]);
  auto s2V = square(sensor.beamSlope()[1]);

  Vector6 prec;
  // offsets in the plane are limited by the resolution
  prec[0] = 1.0 / varU;
  prec[1] = 1.0 / varU;
  // direction along the normal depends on mean beam slope
  prec[2] = (s2U * s2V) / (s2U * varV + s2V * varU);
  prec[3] = prec[2] * l2V;
  prec[4] = prec[2] * l2U;
  // position change equivalent to resolution over the length of the matrix
  prec[5] = (l2U * l2V) / (varU * l2U + varV * l2V);

  VERBOSE(sensor.name(), " alignment sensitivity:");
  VERBOSE("  du: ", rsqrt(prec[0]));
  VERBOSE("  dv: ", rsqrt(prec[1]));
  VERBOSE("  dw: ", rsqrt(prec[2]));
  VERBOSE("  dalpha: ", degree(rsqrt(prec[3])), " degree");
  VERBOSE("  dbeta: ", degree(rsqrt(prec[4])), " degree");
  VERBOSE("  dgamma: ", degree(rsqrt(prec[5])), " degree");
  DEBUG("  beam slope: [", std::sqrt(s2U), ", ", std::sqrt(s2V), "]");

  return prec;
}

static std::vector<size_t> findAlignableParams(const Vector6& precision)
{
  constexpr auto minPrec = 10 * std::numeric_limits<double>::epsilon();

  std::vector<size_t> indices;
  for (size_t i = 0; i < 6; ++i) {
    if (minPrec < precision[i]) {
      indices.push_back(i);
    }
  }
  return indices;
}

// To find the optimal alignment parameters a we linearize the local track
// parameters q as a function of the alignment parameters a, i.e.
//
//     q = q0 + J a
//
// The track residuals are then given by
//
//     res = m - q = (m - q0) - J a = res0 - J a
//
// Finding the optimal alignment parameters is equivalent to minimizing the
// residuals. For this the following least square expression can be defined
//
//     S = (res0 - J a)^T Wr (res0 - J a)
//
// where Wr is the precision matrix of the residuals. Some of the alignment
// parameters can be ill-defined and the resulting normal equations could
// become singular. This can be avoided by using Tikhonov regularization.
// Instead of the initial least square problem we solve the following
// modified one
//
//     S = (res0 - J a)^T Wr (res0 - J a) + a^T Wa a
//
// where Wa is the precision matrix of the alignment parameters. The precision
// matrix can be interpreted as the inverse of a covariance matrix of the
// parameters that defines the scale of possible changes. Minimization of the
// least square expression leads to the following normal equation
//
//     0.5 * dS/da = J^T Wr J a + Wa a - J^T Wr res0
//                 = (J^T Wr J + Wa) a - J^T Wr res0
//                 = F a - y
//                 = 0
//
// that have a direct solution
//
//     a = (J^T Wr J + Wa)^-1 = (Fr + Wa)^-1 y = F^-1 y
//
// with the covariance matrix of the parameters defined as
//
//    Cov = F^-1
//

Alignment::LocalChi2PlaneFitter::LocalChi2PlaneFitter(
    const Vector6& precision, std::vector<size_t> indices)
    : m_numTracks(0), m_subspaceIndices(std::move(indices))
{
  // m_wa, m_fr, m_y are zeroed by default in SMatrix
  m_wa.SetDiagonal(precision);
}

bool Alignment::LocalChi2PlaneFitter::addTrack(
    const Storage::TrackState& track,
    const Storage::Cluster& measurement,
    const SymMatrix2& weight)
{
  // the track fitter sometimes yields NaN as fit values; unclear why, but
  // these values must be ignored otherwise the normal equations become invalid.
  std::array<double, 10> inputs = {
      track.params()[0],
      track.params()[1],
      track.params()[2],
      track.params()[3],
      measurement.posLocal()[0],
      measurement.posLocal()[1],
      weight(0, 0),
      weight(0, 1),
      weight(1, 0),
      weight(1, 1),
  };
  bool allInputsAreFinite = std::all_of(
      inputs.begin(), inputs.end(), [](double x) { return std::isfinite(x); });
  if (!allInputsAreFinite) {
    return false;
  }

  // add finite contribution to the normal equations
  auto jac = jacobianOffsetAlignment(track);
  m_fr += SimilarityT(jac, weight);
  m_y += Transpose(jac) * weight * (measurement.posLocal() - track.offset());
  m_numTracks += 1;
  return true;
}

bool Alignment::LocalChi2PlaneFitter::minimize(Vector6& a,
                                               SymMatrix6& cov) const
{
  // regularize the normal matrix
  SymMatrix6 f = m_fr + m_wa;

  DEBUG("num tracks: ", m_numTracks);
  DEBUG("normal vector:\n", m_y);
  DEBUG("normal matrix:\n", m_fr);
  DEBUG("normal matrix (regularized):\n", f);

  return solveSubspace(f, m_y, m_subspaceIndices, cov, a);
}

Alignment::LocalChi2Aligner::LocalChi2Aligner(
    const Mechanics::Device& device,
    const std::vector<Index>& alignIds,
    const double damping)
    : m_device(device), m_damping(damping)
{
  static const char* const names[] = {
      "du", "dv", "dw", "dalpha", "dbeta", "dgamma",
  };

  for (auto isensor : alignIds) {
    const auto& sensor = device.getSensor(isensor);
    auto precision = estimateAlignmentPrecision(sensor);
    auto indices = findAlignableParams(precision);

    std::string selectedNames;
    for (auto index : indices) {
      selectedNames.append(!selectedNames.empty() ? ", " : "");
      selectedNames.append(names[index]);
    }
    INFO("sensor ", sensor.name(), " alignment parameters: ", selectedNames);

    m_fitters.emplace_back(isensor, LocalChi2PlaneFitter(precision, indices));
  }
}

std::string Alignment::LocalChi2Aligner::name() const
{
  return "LocalChi2Aligner";
}

void Alignment::LocalChi2Aligner::execute(const Storage::Event& event)
{
  for (auto& f : m_fitters) {
    const Storage::SensorEvent& sensorEvent = event.getSensorEvent(f.first);
    LocalChi2PlaneFitter& fitter = f.second;

    for (Index iclu = 0; iclu < sensorEvent.numClusters(); ++iclu) {
      const Storage::Cluster& cluster = sensorEvent.getCluster(iclu);
      if (!cluster.isInTrack())
        continue;
      const Storage::TrackState& state =
          sensorEvent.getLocalState(cluster.track());

      // unbiased residuals have a contribution from
      // the cluster uncertainty and the tracking uncertainty
      SymMatrix2 weight = cluster.covLocal() + state.covOffset();
      if (!weight.InvertChol()) {
        ERROR("Failed to invert cluster covariance event=", event.frame(),
              " sensor=", sensorEvent.sensor(), " track=", cluster.track());
      }
      if (!fitter.addTrack(state, cluster, weight)) {
        ERROR("Invalid track/cluster input event=", event.frame(),
              " sensor=", sensorEvent.sensor(), " track=", cluster.track());
      }
    }
  }
}

Mechanics::Geometry Alignment::LocalChi2Aligner::updatedGeometry() const
{
  Mechanics::Geometry geo = m_device.geometry();

  for (const auto& f : m_fitters) {
    Index isensor = f.first;
    const LocalChi2PlaneFitter& fitter = f.second;
    const Mechanics::Sensor& sensor = m_device.getSensor(isensor);

    // solve the chi2 minimization for optimal parameters
    Vector6 delta;
    SymMatrix6 cov;
    if (!fitter.minimize(delta, cov)) {
      FAIL("Could not solve alignment equations for sensor ", sensor.name());
    }
    delta *= m_damping;

    // output w/ angles in degrees
    Vector6 stddev = sqrt(cov.Diagonal());
    INFO(sensor.name(), " alignment corrections:");
    INFO("  du: ", delta[0], " ± ", stddev[0]);
    INFO("  dv: ", delta[1], " ± ", stddev[1]);
    INFO("  dw: ", delta[2], " ± ", stddev[2]);
    INFO("  dalpha: ", degree(delta[3]), " ± ", degree(stddev[3]), " degree");
    INFO("  dbeta: ", degree(delta[4]), " ± ", degree(stddev[4]), " degree");
    INFO("  dgamma: ", degree(delta[5]), " ± ", degree(stddev[5]), " degree");

    // update sensor geometry
    geo.correctLocal(isensor, delta, cov);
  }
  return geo;
}
