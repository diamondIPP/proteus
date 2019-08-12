// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include "geometry.h"

#include <cassert>
#include <limits>
#include <stdexcept>
#include <string>

#include "tracking/propagation.h"
#include "utils/logger.h"

namespace proteus {

// Construct rotation matrix Q321 = R1(𝛼) * R2(𝛽) * R3(𝛾).
//
// The rotation matrix in 3-2-1 convention mapping the spatial coordinates
// (u,v,w) to the spatial coordinates (x,y,z) is defined as:
//
//            | Qxu  Qxv  Qxw |
//     Q321 = | Qyu  Qyv  Qyw | = R1(𝛼) * R2(𝛽) * R3(𝛾)
//            | Qzu  Qzv  Qzw |
//
// The three angles 𝛾, 𝛽, 𝛼 are right-handed angles around the third, second,
// and first current axis. The resulting matrix can be written as:
//
//     Qxu =          cos(𝛽) cos(𝛾)
//     Qyu =  sin(𝛼) sin(𝛽) cos(𝛾) + cos(𝛼)        sin(𝛾)
//     Qzu =  sin(𝛼)        sin(𝛾) - cos(𝛼) sin(𝛽) cos(𝛾)
//     Qxv =         -cos(𝛽) sin(𝛾)
//     Qyv = -sin(𝛼) sin(𝛽) sin(𝛾) + cos(𝛼)        cos(𝛾)
//     Qzv =  sin(𝛼)        cos(𝛾) + cos(𝛼) sin(𝛽) sin(𝛾)
//     Qxw =  sin(𝛽)
//     Qyw = -sin(𝛼) cos(𝛽)
//     Qzw =  cos(𝛼) cos(𝛽)
//
static Matrix4 makeRotation321(double alpha, double beta, double gamma)
{
  using std::cos;
  using std::sin;

  Matrix4 q = Matrix4::Zero();
  // unit u
  q(kX, kU) = cos(beta) * cos(gamma);
  q(kY, kU) = sin(alpha) * sin(beta) * cos(gamma) + cos(alpha) * sin(gamma);
  q(kZ, kU) = sin(alpha) * sin(gamma) - cos(alpha) * sin(beta) * cos(gamma);
  // unit v
  q(kX, kV) = -cos(beta) * sin(gamma);
  q(kY, kV) = -sin(alpha) * sin(beta) * sin(gamma) + cos(alpha) * cos(gamma);
  q(kZ, kV) = sin(alpha) * cos(gamma) + cos(alpha) * sin(beta) * sin(gamma);
  // unit w
  q(kX, kW) = sin(beta);
  q(kY, kW) = -sin(alpha) * cos(beta);
  q(kZ, kW) = cos(alpha) * cos(beta);
  // time coordinate is not rotated
  q(kT, kS) = 1;
  return q;
};

// Extract rotation angles in 321 convention from 3x3 rotation matrix.
static Vector3 extractAngles321(const Matrix4& q)
{
  // WARNING
  // this is not a stable algorithm and will break down for the case of
  // 𝛽 = ±π, cos(𝛽) = 0, sin(𝛽) = ±1. It should be replaced by a better
  // algorithm. in this code base, only the resulting rotation matrix is used
  // and the angles are only employed for reporting. we should be fine.
  double alpha = std::atan2(-q(kY, kW), q(kZ, kW));
  double beta = std::asin(q(kX, kW));
  double gamma = std::atan2(-q(kX, kV), q(kX, kU));

  // cross-check that we get the same matrix back
  Matrix4 qAngles = makeRotation321(alpha, beta, gamma);
  // Frobenius norm should vanish for correct angle extraction
  auto norm = (Matrix4::Identity() - qAngles.transpose() * q).norm();
  // single epsilon results in too many false-positives.
  if (8 * std::numeric_limits<decltype(norm)>::epsilon() < norm) {
    WARN("detected inconsistent matrix to angles conversion");
    INFO("angles:");
    INFO("  alpha: ", degree(alpha), " degree");
    INFO("  beta: ", degree(beta), " degree");
    INFO("  gamma: ", degree(gamma), " degree");
    INFO("rotation matrix:\n", q);
    INFO("rotation matrix from angles:\n", qAngles);
    INFO("forward-backward distance to identity: ", norm);
  }

  return {alpha, beta, gamma};
}

// Jacobian from small correction angles to full global angles.
//
// Maps small changes [dalpha, dbeta, dgamma] to resulting changes in
// global angles [alpha, beta, gamma]. This is computed by assuming the
// input rotation matrix to the angles extraction to be
//
//     Q'(alpha, beta, gamma) = Q * dQ(dalpha, dbeta, dgamma)  ,
//
// where dQ is the small angle rotation matrix using the correction angles.
// Using the angles extraction defined above the global angles are expressed
// as a function of the corrections and the Jacobian can be calculated.
static Matrix3 jacobianCorrectionsToAngles(const Matrix4& q)
{
  Matrix3 jac;
  // row0: d alpha / d [dalpha, dbeta, dgamma]
  double f0 = q(kY, kW) * q(kY, kW) + q(kZ, kW) * q(kZ, kW);
  jac(0, 0) = (q(kY, kV) * q(kZ, kW) - q(kY, kW) * q(kZ, kV)) / f0;
  jac(0, 1) = (q(kY, kW) * q(kZ, kU) - q(kY, kU) * q(kZ, kW)) / f0;
  jac(0, 2) = 0;
  // row1: d beta / d [dalpha, dbeta, dgamma]
  double f1 = std::sqrt(1.0 - q(kX, kW) * q(kX, kW));
  jac(1, 0) = -q(kX, kV) / f1;
  jac(1, 1) = q(kX, kU) / f1;
  jac(1, 2) = 0;
  // row2: d gamma / d [dalpha, dbeta, dgamma];
  double f2 = q(kX, kU) * q(kX, kU) + q(kX, kV) * q(kX, kV);
  jac(0, 2) = -q(kX, kU) * q(kX, kW) / f2;
  jac(1, 2) = -q(kX, kV) * q(kX, kW) / f2;
  jac(2, 2) = 1;
  return jac;
}

Plane Plane::fromAngles321(double gamma,
                           double beta,
                           double alpha,
                           const Vector3& origin)
{
  Vector4 r0;
  r0[kX] = origin[0];
  r0[kY] = origin[1];
  r0[kZ] = origin[2];
  r0[kT] = 0;
  return {r0, makeRotation321(alpha, beta, gamma)};
}

Plane Plane::fromDirections(const Vector3& dirU,
                            const Vector3& dirV,
                            const Vector3& origin)
{
  // code assumes x, y, z are stored continously
  static_assert(kX + 1 == kY, "Spatial coordinates must be continous");
  static_assert(kX + 2 == kZ, "Spatial coordinates must be continous");
  static_assert(kU + 1 == kV, "Spatial coordinates must be continous");
  static_assert(kU + 2 == kW, "Spatial coordinates must be continous");

  Vector4 r0;
  r0[kX] = origin[0];
  r0[kY] = origin[1];
  r0[kZ] = origin[2];
  r0[kT] = 0;
  Matrix4 q = Matrix4::Zero();
  q.col(kU).segment<3>(kX) = dirU;
  q.col(kV).segment<3>(kX) = dirV;
  q.col(kW).segment<3>(kX) = dirU.cross(dirV);
  q(kT, kS) = 1;
  q.colwise().normalize();
  return {r0, q};
}

Plane Plane::correctedGlobal(const Vector6& delta) const
{
  Vector4 dr;
  dr[kX] = delta[0];
  dr[kY] = delta[1];
  dr[kZ] = delta[2];
  dr[kT] = 0;
  return {m_origin + dr,
          m_linear * makeRotation321(delta[3], delta[4], delta[5])};
}

Plane Plane::correctedLocal(const Vector6& delta) const
{
  Vector4 dr;
  dr[kX] = delta[0];
  dr[kY] = delta[1];
  dr[kZ] = delta[2];
  dr[kT] = 0;
  return {m_origin + m_linear * dr,
          m_linear * makeRotation321(delta[3], delta[4], delta[5])};
}

Vector6 Plane::asParams() const
{
  Vector6 params;
  params[0] = m_origin[kX];
  params[1] = m_origin[kY];
  params[2] = m_origin[kZ];
  params.segment<3>(3) = extractAngles321(m_linear);
  return params;
}

Geometry::Geometry()
    : m_beamSlope(Vector2::Zero())
    , m_beamSlopeStdev(Vector2::Zero())
    , m_beamEnergy(0)
{
}

Geometry Geometry::fromFile(const std::string& path)
{
  auto cfg = configRead(path);
  INFO("read geometry from '", path, "'");
  return fromConfig(cfg);
}

void Geometry::writeFile(const std::string& path) const
{
  configWrite(toConfig(), path);
  INFO("wrote geometry to '", path, "'");
}

Geometry Geometry::fromConfig(const toml::Value& cfg)
{
  Geometry geo;

  // read beam parameters, only beam slope is required
  // stay backward compatible w/ old slope_x/slope_y beam parameters
  if (cfg.has("beam.slope")) {
    auto slope = cfg.get<std::vector<double>>("beam.slope");
    if (slope.size() != 2) {
      FAIL("beam.slope has ", slope.size(), " != 2 entries");
    }
    geo.setBeamSlope({slope[0], slope[1]});
  } else if (cfg.has("beam.slope_x") || cfg.has("beam.slope_y")) {
    ERROR("beam.slope_{x,y} is deprecated, use beam.slope instead");
    geo.setBeamSlope(
        {cfg.get<double>("beam.slope_x"), cfg.get<double>("beam.slope_y")});
  }
  if (cfg.has("beam.divergence")) {
    auto div = cfg.get<std::vector<double>>("beam.divergence");
    if (div.size() != 2) {
      FAIL("beam.divergence has ", div.size(), " != 2 entries");
    }
    if ((div[0] < 0) or (div[1] < 0)) {
      FAIL("beam.divergence must have non-negative values");
    }
    geo.setBeamDivergence({div[0], div[1]});
  }
  if (cfg.has("beam.energy") && !(cfg.has("beam.momentum") || cfg.has("beam.mass"))) {
    geo.m_beamEnergy = cfg.get<double>("beam.energy");
    if(geo.m_beamEnergy < 0.0) FAIL("Negative beam energy");
    //for now flag invalid settings with unphysical numbers (Florian)
    geo.m_beamMomentum = -1.0;
    geo.m_beamMass = -1.0;
  }
  else if(cfg.has("beam.momentum") && cfg.has("beam.mass") && !cfg.has("beam.energy")) {
    //for now flag invalid settings with unphysical numbers (Florian)
    geo.m_beamEnergy = -1.0;
    geo.m_beamMomentum = cfg.get<double>("beam.momentum");
    geo.m_beamMass = cfg.get<double>("beam.mass");
    if(geo.m_beamMomentum < 0.0) FAIL("Negative beam momentum");
    if(geo.m_beamMass < 0.0) FAIL("Negative beam mass");
  } else {
    FAIL("Inconsistent configuration for beam energy or beam mass and momentum");
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
      if ((128 * std::numeric_limits<decltype(projUV)>::epsilon()) < projUV) {
        FAIL("sensor ", sensorId, " has highly non-orthogonal unit vectors");
      } else if ((8 * std::numeric_limits<decltype(projUV)>::epsilon()) <
                 projUV) {
        WARN("sensor ", sensorId, " has non-orthogonal unit vectors");
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

toml::Value Geometry::toConfig() const
{
  toml::Value cfg;

  cfg["beam"]["slope"] = toml::Array{m_beamSlope[0], m_beamSlope[1]};
  cfg["beam"]["divergence"] =
      toml::Array{m_beamSlopeStdev[0], m_beamSlopeStdev[1]};
  if(0.0 < m_beamEnergy && 0.0 > m_beamMomentum && 0.0 > m_beamMass) {
    cfg["beam"]["energy"] = m_beamEnergy;
  }
  else if(0.0 < m_beamMomentum && 0.0 < m_beamMass && 0.0 > m_beamEnergy) {
    cfg["beam"]["momentum"] = m_beamMomentum;
    cfg["beam"]["mass"] = m_beamMass;
  }

  cfg["sensors"] = toml::Array();
  for (const auto& ip : m_planes) {
    Vector4 off = ip.second.origin();
    Vector4 unU = ip.second.linearToGlobal().col(kU);
    Vector4 unV = ip.second.linearToGlobal().col(kV);

    toml::Value cfgSensor;
    cfgSensor["id"] = static_cast<int>(ip.first);
    cfgSensor["offset"] = toml::Array{off[kX], off[kY], off[kZ]};
    cfgSensor["unit_u"] = toml::Array{unU[kX], unU[kY], unU[kZ]};
    cfgSensor["unit_v"] = toml::Array{unV[kX], unV[kY], unV[kZ]};
    cfg["sensors"].push(std::move(cfgSensor));
  }
  return cfg;
}

void Geometry::correctGlobalOffset(Index sensorId,
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

void Geometry::correctGlobal(Index sensorId,
                             const Vector6& delta,
                             const SymMatrix6& cov)
{
  const auto& plane = m_planes.at(sensorId);

  // Jacobian from global corrections to geometry parameters
  Matrix6 jac;
  // clang-format off
  jac << Matrix3::Identity(), Matrix3::Zero(),
         Matrix3::Zero(), jacobianCorrectionsToAngles(plane.linearToGlobal());
  // clang-format on

  m_planes[sensorId] = plane.correctedGlobal(delta);
  m_covs[sensorId] = transformCovariance(jac, cov);
}

void Geometry::correctLocal(Index sensorId,
                            const Vector6& delta,
                            const SymMatrix6& cov)
{
  const auto& plane = m_planes.at(sensorId);

  // Jacobian from local corrections to geometry parameters
  Matrix6 jac;
  // clang-format off
  jac << plane.linearToGlobal().block<3,3>(kX, kX), Matrix3::Zero(),
         Matrix3::Zero(), jacobianCorrectionsToAngles(plane.linearToGlobal());
  // clang-format on

  m_planes[sensorId] = plane.correctedLocal(delta);
  m_covs[sensorId] = transformCovariance(jac, cov);
}

const Plane& Geometry::getPlane(Index sensorId) const
{
  return m_planes.at(sensorId);
}

Vector6 Geometry::getParams(Index sensorId) const
{
  return m_planes.at(sensorId).asParams();
}

SymMatrix6 Geometry::getParamsCov(Index sensorId) const
{
  auto it = m_covs.find(sensorId);
  if (it != m_covs.end()) {
    return it->second;
  }
  return SymMatrix6::Zero();
}

Vector4 Geometry::beamTangent() const
{
  Vector4 tangent;
  tangent[kX] = m_beamSlope[0];
  tangent[kY] = m_beamSlope[1];
  tangent[kZ] = 1.0;
  tangent[kT] = 0;
  return tangent;
}

SymMatrix2 Geometry::beamSlopeCovariance() const
{
  return m_beamSlopeStdev.cwiseProduct(m_beamSlopeStdev).asDiagonal();
}

Vector2 Geometry::getBeamSlope(Index sensorId) const
{
  Vector4 tgtLocal = m_planes.at(sensorId).linearToLocal() * beamTangent();
  Vector2 slopeLocal(tgtLocal[kU] / tgtLocal[kW], tgtLocal[kV] / tgtLocal[kW]);
  DEBUG("global beam tangent: [", beamTangent().transpose(), "]");
  DEBUG("sensor ", sensorId, " beam tangent: [", tgtLocal.transpose(), "]");
  DEBUG("sensor ", sensorId, " beam slope: [", slopeLocal.transpose(), "]");
  return slopeLocal;
}

SymMatrix2 Geometry::getBeamSlopeCovariance(Index sensorId) const
{
  const auto& plane = m_planes.at(sensorId);
  auto jac = jacobianSlopeSlope(beamTangent(), plane.linearToGlobal());
  SymMatrix2 cov = transformCovariance(jac, beamSlopeCovariance());
  DEBUG("global beam divergence: [",
        extractStdev(beamSlopeCovariance()).transpose(), "]");
  DEBUG("global beam covariance:\n", beamSlopeCovariance());
  DEBUG("global to sensor ", sensorId, " slope jacobian:\n", jac);
  DEBUG("sensor ", sensorId, " beam covariance:\n", cov);
  DEBUG("sensor ", sensorId, " beam divergence: [",
        extractStdev(cov).transpose(), "]");
  return cov;
}

void Geometry::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "beam:\n";
  os << prefix << "  energy: " << m_beamEnergy << '\n';
  os << prefix << "  slope: " << format(m_beamSlope) << '\n';
  os << prefix << "  divergence: " << format(m_beamSlopeStdev) << '\n';
  for (const auto& ip : m_planes) {
    const auto& sensorId = ip.first;
    const auto& plane = ip.second;

    os << prefix << "sensor " << sensorId << ":\n";

    Vector3 r0(plane.origin()[kX], plane.origin()[kY], plane.origin()[kZ]);
    os << prefix << "  offset: " << format(r0) << '\n';

    auto Q = plane.linearToGlobal();
    Vector3 unitU(Q(kU, kX), Q(kU, kY), Q(kU, kZ));
    Vector3 unitV(Q(kV, kX), Q(kV, kY), Q(kV, kZ));
    Vector3 unitW(Q(kW, kX), Q(kW, kY), Q(kW, kZ));
    os << prefix << "  unit u: " << format(unitU) << '\n';
    os << prefix << "  unit v: " << format(unitV) << '\n';
    os << prefix << "  unit w: " << format(unitW) << '\n';

    Vector3 angles = plane.asParams().segment<3>(3);
    for (size_t i = 0; i < 3; ++i) {
      angles[i] = degree(angles[i]);
    }
    os << prefix << "  angles: " << format(angles) << '\n';

    Vector2 beamSlope = getBeamSlope(sensorId);
    Vector2 beamDivergence = extractStdev(getBeamSlopeCovariance(sensorId));
    os << prefix << "  beam:\n";
    os << prefix << "    slope: " << format(beamSlope) << '\n';
    os << prefix << "    divergence: " << format(beamDivergence) << '\n';
  }
  os.flush();
}

void sortAlongBeam(const Geometry& geo, std::vector<Index>& sensorIds)
{
  // TODO 2017-10 msmk: actually sort along beam direction and not just along
  //                    z-axis as proxy.
  std::sort(sensorIds.begin(), sensorIds.end(), [&](Index id0, Index id1) {
    return geo.getPlane(id0).origin()[kZ] < geo.getPlane(id1).origin()[kZ];
  });
}

std::vector<Index> sortedAlongBeam(const Geometry& geo,
                                   const std::vector<Index>& sensorIds)
{
  std::vector<Index> sorted(std::begin(sensorIds), std::end(sensorIds));
  sortAlongBeam(geo, sorted);
  return sorted;
}

} // namespace proteus
