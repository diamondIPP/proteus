#include "geometry.h"

#include <cassert>
#include <limits>
#include <stdexcept>
#include <string>

#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Geometry)

// compute the nearest orthogonal matrix.
//
// assumes the matrix already is or is close to an orthogonal matrix and uses
// the interative algorithm outlined at
// https://en.wikipedia.org/wiki/Orthogonal_matrix to orthogonalize it.
static Matrix3 orthogonalize(const Matrix3& m, size_t iterations = 2)
{
  Matrix3 q = m;
  for (size_t i = 0; i < iterations; ++i) {
    Matrix3 n = q.transpose() * q;
    Matrix3 p = 0.5 * q * n;
    Matrix3 qi = 2 * q + p * n - 3 * p;
    DEBUG("orthogonal correction ", i, ":\n", q - qi);
    q = qi;
  }
  return q;
}

// Most of the code uses the rotation matrix for all geometric computations.
// For cases where the minimal set of three angles is needed the
// 3-2-1 convention is used to compute the rotation matrix.
//
//            | Q00  Q01  Q02 |
//     Q321 = | Q10  Q11  Q12 | = R1(𝛼) * R2(𝛽) * R3(𝛾)
//            | Q20  Q21  Q22 |
//
// The three angles 𝛾, 𝛽, 𝛼 are right-handed angles around the third, second,
// and first current axis. The resulting matrix can be written as:
//
//     Q00 =          cos(𝛽) cos(𝛾)
//     Q10 =  sin(𝛼) sin(𝛽) cos(𝛾) + cos(𝛼)        sin(𝛾)
//     Q20 =  sin(𝛼)        sin(𝛾) - cos(𝛼) sin(𝛽) cos(𝛾)
//     Q01 =         -cos(𝛽) sin(𝛾)
//     Q11 = -sin(𝛼) sin(𝛽) sin(𝛾) + cos(𝛼)        cos(𝛾)
//     Q21 =  sin(𝛼)        cos(𝛾) + cos(𝛼) sin(𝛽) sin(𝛾)
//     Q02 =  sin(𝛽)
//     Q12 = -sin(𝛼) cos(𝛽)
//     Q22 =  cos(𝛼) cos(𝛽)
//

// Construct R321 = R1(alpha)*R2(beta)*R3(gamma) rotation matrix from angles.
static Matrix3 makeRotation321(double gamma, double beta, double alpha)
{
  using std::cos;
  using std::sin;

  Matrix3 q;
  // column 0
  q(0, 0) = cos(beta) * cos(gamma);
  q(1, 0) = sin(alpha) * sin(beta) * cos(gamma) + cos(alpha) * sin(gamma);
  q(2, 0) = sin(alpha) * sin(gamma) - cos(alpha) * sin(beta) * cos(gamma);
  // column 1
  q(0, 1) = -cos(beta) * sin(gamma);
  q(1, 1) = -sin(alpha) * sin(beta) * sin(gamma) + cos(alpha) * cos(gamma);
  q(2, 1) = sin(alpha) * cos(gamma) + cos(alpha) * sin(beta) * sin(gamma);
  // column 2
  q(0, 2) = sin(beta);
  q(1, 2) = -sin(alpha) * cos(beta);
  q(2, 2) = cos(alpha) * cos(beta);
  return q;
};

struct Angles321 {
  double alpha = 0.0;
  double beta = 0.0;
  double gamma = 0.0;
};

// Extract rotation angles in 321 convention from rotation matrix.
static Angles321 extractAngles321(const Matrix3& q)
{
  // WARNING
  // this is not a stable algorithm and will break down for the case of
  // 𝛽 = ±π, cos(𝛽) = 0, sin(𝛽) = ±1. It should be replaced by a better
  // algorithm. in this code base, the rotation matrix is used and stored
  // and the angles are only used for reporting. we should be fine.
  Angles321 angles;
  angles.alpha = std::atan2(-q(1, 2), q(2, 2));
  angles.beta = std::asin(q(0, 2));
  angles.gamma = std::atan2(-q(0, 1), q(0, 0));

  // cross-check that we get the same matrix back
  Matrix3 qFromAngles =
      makeRotation321(angles.gamma, angles.beta, angles.alpha);
  // Frobenius norm should vanish for correct angle extraction
  auto norm = (Matrix3::Identity() - qFromAngles.transpose() * q).norm();
  // single epsilon results in too many false-positives.
  if (4 * std::numeric_limits<decltype(norm)>::epsilon() < norm) {
    constexpr double toDeg = 180.0 / M_PI;
    ERROR("detected inconsistent matrix to angles conversion");
    INFO("angles:");
    INFO("  alpha: ", angles.alpha * toDeg, " degree");
    INFO("  beta: ", angles.beta * toDeg, " degree");
    INFO("  gamma: ", angles.gamma * toDeg, " degree");
    INFO("rotation matrix:\n", q);
    INFO("rotation matrix from angles:\n", qFromAngles);
    INFO("forward-backward distance to identity: ", norm);
  }

  return angles;
}

// Jacobian from correction angles to global
//
// Maps small changes [dalpha, dbeta, dgamma] to resulting changes in
// global angles [alpha, beta, gamma]. This is computed by assuming the
// input rotation matrix to the angles extraction to be
//
//     Q' = Q * dQ
//
// where dQ is the small angle rotation matrix using the correction angles.
// Using the angles extraction defined above the global angles are expressed
// as a function of the corrections and the Jacobian can be calculated.
static Matrix3 jacobianCorrectionsToAngles(const Matrix3 q)
{
  Matrix3 jac;
  // d alpha / d [dalpha, dbeta, dgamma]
  double f0 = q(1, 2) * q(1, 2) + q(2, 2) * q(2, 2);
  jac(0, 0) = (q(1, 1) * q(2, 2) - q(1, 2) * q(2, 1)) / f0;
  jac(1, 0) = (q(1, 2) * q(2, 0) - q(1, 0) * q(2, 2)) / f0;
  jac(2, 0) = 0;
  // d beta / d [dalpha, dbeta, dgamma]
  double f1 = std::sqrt(1.0 - q(0, 2) * q(0, 2));
  jac(0, 1) = -q(0, 1) / f1;
  jac(1, 1) = q(0, 0) / f1;
  jac(2, 1) = 0;
  // d gamma / d [dalpha, dbeta, dgamma];
  double f2 = q(0, 0) * q(0, 0) + q(0, 1) * q(0, 1);
  jac(0, 2) = -q(0, 0) * q(0, 2) / f2;
  jac(1, 2) = -q(0, 1) * q(0, 2) / f2;
  jac(2, 2) = 1;
  return jac;
}

Mechanics::Plane Mechanics::Plane::fromAngles321(double gamma,
                                                 double beta,
                                                 double alpha,
                                                 const Vector3& offset)
{
  Plane p;
  p.m_rotation = makeRotation321(gamma, beta, alpha);
  p.m_offset = offset;
  return p;
}

Mechanics::Plane Mechanics::Plane::fromDirections(const Vector3& dirU,
                                                  const Vector3& dirV,
                                                  const Vector3& offset)
{
  Plane p;
  p.m_rotation.col(0) = dirU.normalized();
  p.m_rotation.col(1) = dirV.normalized();
  p.m_rotation.col(2) = dirU.normalized().cross(dirV.normalized());
  p.m_rotation = orthogonalize(p.m_rotation);
  p.m_offset = offset;
  return p;
}

Mechanics::Plane Mechanics::Plane::correctedGlobal(const Vector6& delta) const
{
  Plane corrected;
  corrected.m_rotation =
      m_rotation * makeRotation321(delta[5], delta[4], delta[3]);
  corrected.m_rotation = orthogonalize(corrected.m_rotation);
  corrected.m_offset = m_offset + delta.head<3>();
  return corrected;
}

Mechanics::Plane Mechanics::Plane::correctedLocal(const Vector6& delta) const
{
  Plane corrected;
  corrected.m_rotation =
      m_rotation * makeRotation321(delta[5], delta[4], delta[3]);
  corrected.m_rotation = orthogonalize(corrected.m_rotation);
  // local offset is defined in the old system
  corrected.m_offset = m_offset + m_rotation * delta.head<3>();
  return corrected;
}

Vector6 Mechanics::Plane::asParams() const
{
  Vector6 params;
  params[0] = m_offset[0];
  params[1] = m_offset[1];
  params[2] = m_offset[2];
  auto angles = extractAngles321(m_rotation);
  params[3] = angles.alpha;
  params[4] = angles.beta;
  params[5] = angles.gamma;
  return params;
}

Mechanics::Geometry::Geometry()
    : m_beamSlopeX(0)
    , m_beamSlopeY(0)
    , m_beamDivergenceX(0.00125)
    , m_beamDivergenceY(0.00125)
    , m_beamEnergy(0)
{
}

Mechanics::Geometry Mechanics::Geometry::fromFile(const std::string& path)
{
  auto cfg = Utils::Config::readConfig(path);
  INFO("read geometry from '", path, "'");
  return fromConfig(cfg);
}

void Mechanics::Geometry::writeFile(const std::string& path) const
{
  Utils::Config::writeConfig(toConfig(), path);
  INFO("wrote geometry to '", path, "'");
}

Mechanics::Geometry Mechanics::Geometry::fromConfig(const toml::Value& cfg)
{
  Geometry geo;

  // read beam parameters, only beam slope is required
  // stay backward compatible w/ old slope_x/slope_y beam parameters
  if (cfg.has("beam.slope")) {
    auto slope = cfg.get<std::vector<double>>("beam.slope");
    if (slope.size() != 2)
      FAIL("beam.slope has ", slope.size(), " != 2 entries");
    geo.setBeamSlope(slope[0], slope[1]);
  } else if (cfg.has("beam.slope_x") || cfg.has("beam.slope_y")) {
    ERROR("beam.slope_{x,y} is deprecated, use beam.slope instead");
    geo.setBeamSlope(cfg.get<double>("beam.slope_x"),
                     cfg.get<double>("beam.slope_y"));
  }
  if (cfg.has("beam.divergence")) {
    auto div = cfg.get<std::vector<double>>("beam.divergence");
    if (div.size() != 2)
      FAIL("beam.divergence has ", div.size(), " != 2 entries");
    if (!(0 < div[0]) || !(0 < div[1]))
      FAIL("beam.divergence must have non-zero, positive values");
    geo.setBeamDivergence(div[0], div[1]);
  }
  if (cfg.has("beam.energy")) {
    geo.m_beamEnergy = cfg.get<double>("beam.energy");
  }

  auto sensors = cfg.get<toml::Array>("sensors");
  for (const auto& cs : sensors) {
    auto sensorId = cs.get<int>("id");

    if (cs.has("offset")) {
      auto off = cs.get<std::vector<double>>("offset");
      auto unU = cs.get<std::vector<double>>("unit_u");
      auto unV = cs.get<std::vector<double>>("unit_v");

      if (off.size() != 3)
        FAIL("sensor ", sensorId, " has offset number of entries != 3");
      if (unU.size() != 3)
        FAIL("sensor ", sensorId, " has unit_u number of entries != 3");
      if (unV.size() != 3)
        FAIL("sensor ", sensorId, " has unitv_ number of entries != 3");

      Vector3 unitU(unU[0], unU[1], unU[2]);
      Vector3 unitV(unV[0], unV[1], unV[2]);
      Vector3 offset(off[0], off[1], off[2]);
      auto projUV = std::abs(unitU.normalized().dot(unitV.normalized()));

      DEBUG("sensor ", sensorId, " unit vector projection ", projUV);
      // approximate zero check; the number of ignored bits is a bit arbitrary
      if ((4 * std::numeric_limits<decltype(projUV)>::epsilon()) < projUV) {
        FAIL("sensor ", sensorId, " has non-orthogonal unit vectors");
      }

      geo.m_planes[sensorId] = Plane::fromDirections(unitU, unitV, offset);
    } else {
      auto rotX = cs.get<double>("rotation_x");
      auto rotY = cs.get<double>("rotation_y");
      auto rotZ = cs.get<double>("rotation_z");
      auto offX = cs.get<double>("offset_x");
      auto offY = cs.get<double>("offset_y");
      auto offZ = cs.get<double>("offset_z");
      geo.m_planes[sensorId] =
          Plane::fromAngles321(rotZ, rotY, rotX, {offX, offY, offZ});
    }
  }
  return geo;
}

toml::Value Mechanics::Geometry::toConfig() const
{
  toml::Value cfg;

  cfg["beam"]["slope"] = toml::Array{m_beamSlopeX, m_beamSlopeY};
  cfg["beam"]["divergence"] = toml::Array{m_beamDivergenceX, m_beamDivergenceY};
  cfg["beam"]["energy"] = m_beamEnergy;

  cfg["sensors"] = toml::Array();
  for (const auto& ip : m_planes) {
    int id = static_cast<int>(ip.first);
    Vector3 unU = ip.second.unitU();
    Vector3 unV = ip.second.unitV();
    Vector3 off = ip.second.offset();

    toml::Value cfgSensor;
    cfgSensor["id"] = id;
    cfgSensor["offset"] = toml::Array{off[0], off[1], off[2]};
    cfgSensor["unit_u"] = toml::Array{unU[0], unU[1], unU[2]};
    cfgSensor["unit_v"] = toml::Array{unV[0], unV[1], unV[2]};
    cfg["sensors"].push(std::move(cfgSensor));
  }
  return cfg;
}

void Mechanics::Geometry::correctGlobalOffset(Index sensorId,
                                              double dx,
                                              double dy,
                                              double dz)
{
  Vector6 delta;
  delta[0] = dx;
  delta[1] = dy;
  delta[2] = dz;
  delta[3] = 0.0;
  delta[4] = 0.0;
  delta[5] = 0.0;
  auto& plane = m_planes.at(sensorId);
  plane = plane.correctedGlobal(delta);
}

void Mechanics::Geometry::correctGlobal(Index sensorId,
                                        const Vector6& delta,
                                        const SymMatrix6& cov)
{
  const auto& plane = m_planes.at(sensorId);

  // Jacobian from global corrections to geometry parameters
  Matrix6 jac;
  // clang-format off
  jac << Matrix3::Identity(), Matrix3::Zero(),
         Matrix3::Zero(), jacobianCorrectionsToAngles(plane.rotationToGlobal());
  // clang-format on

  m_planes[sensorId] = plane.correctedLocal(delta);
  m_covs[sensorId] = jac * cov * jac.transpose();
}

void Mechanics::Geometry::correctLocal(Index sensorId,
                                       const Vector6& delta,
                                       const SymMatrix6& cov)
{
  const auto& plane = m_planes.at(sensorId);

  // Jacobian from local corrections to geometry parameters
  Matrix6 jac;
  // clang-format off
  jac << plane.rotationToGlobal(), Matrix3::Zero(),
         Matrix3::Zero(), jacobianCorrectionsToAngles(plane.rotationToGlobal());
  // clang-format on

  m_planes[sensorId] = plane.correctedLocal(delta);
  m_covs[sensorId] = jac * cov * jac.transpose();
}

const Mechanics::Plane& Mechanics::Geometry::getPlane(Index sensorId) const
{
  return m_planes.at(sensorId);
}

Vector6 Mechanics::Geometry::getParams(Index sensorId) const
{
  return m_planes.at(sensorId).asParams();
}

SymMatrix6 Mechanics::Geometry::getParamsCov(Index sensorId) const
{
  auto it = m_covs.find(sensorId);
  if (it != m_covs.end()) {
    return it->second;
  } else {
    return SymMatrix6(); // zero by default
  }
}

void Mechanics::Geometry::setBeamSlope(double slopeX, double slopeY)
{
  m_beamSlopeX = slopeX;
  m_beamSlopeY = slopeY;
}

void Mechanics::Geometry::setBeamDivergence(double divergenceX,
                                            double divergenceY)
{
  m_beamDivergenceX = divergenceX;
  m_beamDivergenceY = divergenceY;
}

Vector3 Mechanics::Geometry::beamDirection() const
{
  return Vector3(m_beamSlopeX, m_beamSlopeY, 1);
}

Vector2 Mechanics::Geometry::beamDivergence() const
{
  return Vector2(m_beamDivergenceX, m_beamDivergenceY);
}

void Mechanics::Geometry::print(std::ostream& os,
                                const std::string& prefix) const
{
  os << prefix << "beam:\n";
  os << prefix << "  energy: " << m_beamEnergy << '\n';
  os << prefix << "  slope: [" << m_beamSlopeX << "," << m_beamSlopeY << "]\n";
  os << prefix << "  divergence: [" << m_beamDivergenceX << ","
     << m_beamDivergenceY << "]\n";
  for (const auto& ip : m_planes) {
    os << prefix << "sensor " << ip.first << ":\n"
       << prefix << "  offset: [" << ip.second.offset() << "]\n"
       << prefix << "  unit u: [" << ip.second.unitU() << "]\n"
       << prefix << "  unit v: [" << ip.second.unitV() << "]\n"
       << prefix << "  unit w: [" << ip.second.unitNormal() << "]\n";
  }
  os.flush();
}

std::vector<Index>
Mechanics::sortedAlongBeam(const Mechanics::Geometry& geo,
                           const std::vector<Index>& sensorIds)
{
  // TODO 2017-10 msmk: actually sort along beam direction and not just along
  //                    z-axis as proxy.
  std::vector<Index> sorted(std::begin(sensorIds), std::end(sensorIds));
  std::sort(sorted.begin(), sorted.end(), [&](Index id0, Index id1) {
    return geo.getPlane(id0).offset()[2] < geo.getPlane(id1).offset()[2];
  });
  return sorted;
}
