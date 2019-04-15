#include "localchi2aligner.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

#include <Eigen/SVD>

#include "mechanics/sensor.h"
#include "storage/event.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(LocalChi2Aligner)

// local helper functions

// Map [du, dv, dw, dalpha, dbeta, dgamma] to track offset changes.
//
// Assumes the track will stay the constant in the global frame and the
// alignment corrections result in a different intersection point.
//
// This Jacobian is equivalent to eq. 17 from V. Karimaeki et al., 2003,
// arXiv:physics/0306034 w/ some sign modifications to adjust for
// a different alignment parameter convention (see Geometry::Plane).
static Matrix<Scalar, 2, 6>
jacobianOffsetAlignment(const Storage::TrackState& state)
{
  auto u = state.loc0();
  auto v = state.loc1();
  auto slopeU = state.slopeLoc0();
  auto slopeV = state.slopeLoc1();

  Matrix<Scalar, 2, 6> jac;
  jac(0, 0) = -1;          // d u / d du
  jac(0, 1) = 0;           // d u / d dv
  jac(0, 2) = slopeU;      // d u / d dw
  jac(0, 3) = slopeU * v;  // d u / d dalpha
  jac(0, 4) = -slopeU * u; // d u / d dbeta
  jac(0, 5) = v;           // d u / d dgamma
  jac(1, 0) = 0;           // d v / d du
  jac(1, 1) = -1;          // d v / d dv
  jac(1, 2) = slopeV;      // d v / d dw
  jac(1, 3) = slopeV * v;  // d v / d dalpha
  jac(1, 4) = -slopeV * u; // d v / d dbeta
  jac(1, 5) = -u;          // d v / d dgamma
  return jac;
}

// For regularization it's better to have all the parameters with the same
// units. This function returns the matrix that converts the rotation angles to
// distances (using the sensor dimensions). It must be applied to the jacobian
// and later to the covariance matrix and to the offset vector.

static DiagMatrix6 jacobianScaling(const Mechanics::Sensor& sensor)
{
  auto box = sensor.sensitiveVolume();
  auto l_alpha = box.length(kU);
  auto l_beta = box.length(kV);
  auto l_gamma = std::hypot(l_alpha, l_beta);

  DiagMatrix6 scaling;
  scaling.diagonal() << 1., 1., 1., 1 / l_alpha, 1 / l_beta, 1 / l_gamma;

  return scaling;
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
// Or you can switch to a good linear algebra library with a working and
// robust singular value decomposition, ignore vanishing singular values,
// and do not have to bother with the whole regularization scheme at all.

Alignment::LocalChi2PlaneFitter::LocalChi2PlaneFitter(
    const DiagMatrix6& scaling)
    : m_scaling(scaling)
    , m_fr(SymMatrix6::Zero())
    , m_y(Vector6::Zero())
    , m_numTracks(0)
{
}

bool Alignment::LocalChi2PlaneFitter::addTrack(
    const Storage::TrackState& track,
    const Storage::Cluster& measurement,
    const SymMatrix2& weight)
{
  // the track fitter sometimes yields NaN as fit values; unclear why, but
  // these values must be ignored otherwise the normal equations become invalid.
  double inputs[] = {
      track.loc0(),      track.loc1(),      track.time(),    track.slopeLoc0(),
      track.slopeLoc1(), track.slopeTime(), measurement.u(), measurement.v(),
      weight(0, 0),      weight(0, 1),      weight(1, 0),    weight(1, 1),
  };
  bool allInputsAreFinite =
      std::all_of(std::begin(inputs), std::end(inputs),
                  [](double x) { return std::isfinite(x); });
  if (!allInputsAreFinite) {
    return false;
  }

  // add finite contribution to the normal equations
  auto jac = jacobianOffsetAlignment(track);
  // m_scaling is diagonal, no need to transpose it
  m_fr += m_scaling * jac.transpose() * weight * jac * m_scaling;
  m_y +=
      m_scaling * jac.transpose() * weight *
      Vector2(measurement.u() - track.loc0(), measurement.v() - track.loc1());
  m_numTracks += 1;
  return true;
}

template <typename Unit>
static inline std::string
printEffectiveParameter(const Eigen::MatrixBase<Unit>& unit)
{
  constexpr size_t kSize = 6;
  constexpr double kCutoff = 0.001;
  // clang-format off
  static const char* kNames[] = {
      "du    ",
      "dv    ",
      "dw    ",
      "dalpha",
      "dbeta ",
      "dgamma",
  };
  // clang-format on

  // sort contributions by absolute size, largest first
  std::array<size_t, kSize> order;
  std::iota(order.begin(), order.end(), 0);
  std::sort(order.begin(), order.end(),
            [&](size_t i, size_t j) { return unit[i] > unit[j]; });

  // print linear combinations and ignore small contributions
  std::string out;
  for (size_t i : order) {
    double value = unit[i];
    if (std::abs(value) <= kCutoff) {
      continue;
    }
    if (!out.empty()) {
      out += ' ';
    }
    char valueStr[8];
    std::snprintf(valueStr, 8, "%+6.3f", value);
    out += valueStr;
    out += ' ';
    out += kNames[i];
  }
  return out;
}

bool Alignment::LocalChi2PlaneFitter::minimize(Vector6& a,
                                               SymMatrix6& cov) const
{
  DEBUG("num tracks: ", m_numTracks);
  DEBUG("normal vector:\n", m_y);
  DEBUG("normal matrix:\n", m_fr);

  Eigen::JacobiSVD<Matrix6, Eigen::NoQRPreconditioner> svd(
      m_fr, Eigen::ComputeFullU | Eigen::ComputeFullV);

  // ignore small singular values that correspond to weak modes
  // the default value of just epsilon is not large enough to handle weak modes
  // TODO the threshold probably needs to be dependent on tel/dut. Set it from
  // the conf file?
  svd.setThreshold(1e-6);

  VERBOSE("singular values:\n", svd.singularValues().transpose());
  VERBOSE("U^T y:\n", (svd.matrixU().transpose() * m_y).transpose());
  VERBOSE("threshold: ", svd.threshold());
  for (size_t i = 0; i < static_cast<size_t>(svd.rank()); ++i) {
    VERBOSE("effective parameter: ",
            printEffectiveParameter(svd.matrixU().col(i)));
  }

  // revert internal parameter scaling for output
  a = m_scaling * svd.solve(m_y);
  cov = transformCovariance(m_scaling, svd.solve(Matrix6::Identity()));

  // we should have at least 2 effective parameters
  return (2 <= svd.rank());
}

Alignment::LocalChi2Aligner::LocalChi2Aligner(
    const Mechanics::Device& device,
    const std::vector<Index>& alignIds,
    const double damping)
    : m_device(device), m_damping(damping)
{
  for (auto isensor : alignIds) {
    m_fitters.emplace_back(isensor, LocalChi2PlaneFitter(jacobianScaling(
                                        m_device.getSensor(isensor))));
  }
}

std::string Alignment::LocalChi2Aligner::name() const
{
  return "LocalChi2Aligner";
}

void Alignment::LocalChi2Aligner::execute(const Storage::Event& event)
{
  for (auto& f : m_fitters) {
    auto sensorId = f.first;
    const Storage::SensorEvent& sensorEvent = event.getSensorEvent(sensorId);
    LocalChi2PlaneFitter& fitter = f.second;

    for (Index iclu = 0; iclu < sensorEvent.numClusters(); ++iclu) {
      const Storage::Cluster& cluster = sensorEvent.getCluster(iclu);
      if (!cluster.isInTrack())
        continue;
      const Storage::TrackState& state =
          sensorEvent.getLocalState(cluster.track());

      // unbiased residuals have a contribution from
      // the cluster uncertainty and the tracking uncertainty
      SymMatrix2 weight = (cluster.uvCov() + state.loc01Cov()).inverse();
      if (!fitter.addTrack(state, cluster, weight)) {
        ERROR("Invalid track/cluster input event=", event.frame(),
              " sensor=", sensorId, " track=", cluster.track());
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
    Vector6 stddev = extractStdev(cov);
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
