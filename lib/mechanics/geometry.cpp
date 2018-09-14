#include "geometry.h"

#include <cassert>
#include <stdexcept>
#include <string>

#include <Math/GenVector/Rotation3D.h>
#include <Math/GenVector/RotationZYX.h>

#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Geometry)

// Construct R1(alpha) * R2(beta) * R3(gamma) rotation matrix.
//
// see also: https://en.wikipedia.org/wiki/Rotation_matrix#Basic_rotations
static Matrix3 makeRotation321(double gamma, double beta, double alpha)
{
  Matrix3 rot;
  Matrix3 tmp;
  // R1(alpha), elementary right-handed rotation around first axis
  // column 0
  rot(0, 0) = 1;
  rot(1, 0) = 0;
  rot(2, 0) = 0;
  // column 1
  rot(0, 1) = 0;
  rot(1, 1) = std::cos(alpha);
  rot(2, 1) = std::sin(alpha);
  // column 2
  rot(0, 2) = 0;
  rot(1, 2) = -std::sin(alpha);
  rot(2, 2) = std::cos(alpha);
  // R2(beta), elementary right-handed rotation around second axis
  // column 0
  tmp(0, 0) = std::cos(beta);
  tmp(1, 0) = 0;
  tmp(2, 0) = -std::sin(beta);
  // column 1
  tmp(0, 1) = 0;
  tmp(1, 1) = 1;
  tmp(2, 1) = 0;
  // column 2
  tmp(0, 2) = std::sin(alpha);
  tmp(1, 2) = 0;
  tmp(2, 2) = std::cos(alpha);
  rot *= tmp;
  // R3(gamma), elementary right-handed rotation around third axis
  // column 0
  tmp(0, 0) = std::cos(gamma);
  tmp(1, 0) = std::sin(gamma);
  tmp(2, 0) = 0;
  // column 1
  tmp(0, 1) = -std::sin(gamma);
  tmp(1, 1) = std::cos(gamma);
  tmp(2, 1) = 0;
  // column 2
  tmp(0, 2) = 0;
  tmp(1, 2) = 0;
  tmp(2, 2) = 1;
  rot *= tmp;
  return rot;
};

struct Angles321 {
  double alpha = 0.0;
  double beta = 0.0;
  double gamma = 0.0;
};

// Extract rotation angles in 321 convention from rotation matrix.
static Angles321 extractAngles321(const Matrix3& rotation)
{
  // TODO remove ROOT dependency and implement matrix to angles directly. See:
  // https://project-mathlibs.web.cern.ch/project-mathlibs/documents/eulerAngleComputation.pdf
  ROOT::Math::Rotation3D rot;
  rot.SetRotationMatrix(rotation);
  ROOT::Math::RotationZYX zyx(rot);

  Angles321 angles;
  angles.alpha = zyx.Psi();
  angles.beta = zyx.Theta();
  angles.gamma = zyx.Phi();
  return angles;
}

Mechanics::Plane Mechanics::Plane::fromAnglesZYX(double gamma,
                                                 double beta,
                                                 double alpha,
                                                 const Vector3& offset)
{
  Plane p;
  p.rotation = makeRotation321(gamma, beta, alpha);
  p.offset = offset;
  return p;
}

Mechanics::Plane Mechanics::Plane::fromDirections(const Vector3& dirU,
                                                  const Vector3& dirV,
                                                  const Vector3& offset)
{
  Plane p;
  p.rotation.Place_in_col(Unit(dirU), 0, 0);
  p.rotation.Place_in_col(Unit(dirV), 0, 1);
  p.rotation.Place_in_col(Unit(Cross(dirU, dirV)), 0, 2);
  p.offset = offset;
  return p;
}

Mechanics::Plane Mechanics::Plane::correctedGlobal(const Vector6& delta) const
{
  Plane corrected;
  corrected.rotation = rotation * makeRotation321(delta[5], delta[4], delta[3]);
  corrected.offset = offset + delta.Sub<Vector3>(0);

  DEBUG("corrected rotation:");
  DEBUG("  dot(u,v): ", Dot(corrected.unitU(), corrected.unitV()));
  DEBUG("  dot(u,w): ", Dot(corrected.unitU(), corrected.unitNormal()));
  DEBUG("  dot(v,w): ", Dot(corrected.unitV(), corrected.unitNormal()));
  return corrected;
}

Mechanics::Plane Mechanics::Plane::correctedLocal(const Vector6& delta) const
{
  Plane corrected;
  corrected.rotation = rotation * makeRotation321(delta[5], delta[4], delta[3]);
  corrected.offset = offset + rotation * delta.Sub<Vector3>(0);

  DEBUG("corrected rotation:");
  DEBUG("  dot(u,v): ", Dot(corrected.unitU(), corrected.unitV()));
  DEBUG("  dot(u,w): ", Dot(corrected.unitU(), corrected.unitNormal()));
  DEBUG("  dot(v,w): ", Dot(corrected.unitV(), corrected.unitNormal()));
  return corrected;
}

Vector6 Mechanics::Plane::asParams() const
{
  Vector6 params;
  params[0] = offset[0];
  params[1] = offset[1];
  params[2] = offset[2];
  auto angles = extractAngles321(rotation);
  params[3] = angles.alpha;
  params[4] = angles.beta;
  params[5] = angles.gamma;
  return params;
}

Vector3 Mechanics::Plane::toLocal(const Vector3& xyz) const
{
  return Transpose(rotation) * (xyz - offset);
}

Vector3 Mechanics::Plane::toGlobal(const Vector2& uv) const
{
  return offset + rotation.Sub<Matrix32>(0, 0) * uv;
}

Vector3 Mechanics::Plane::toGlobal(const Vector3& uvw) const
{
  return offset + rotation * uvw;
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
      double dot = Dot(unitU.Unit(), unitV.Unit());

      DEBUG("sensor ", sensorId, " unit vector projection ", dot);
      // approximate zero check; the number of ignored bits is a bit arbitrary
      if ((4 * std::numeric_limits<double>::epsilon()) < std::abs(dot))
        FAIL("sensor ", sensorId, " has non-orthogonal unit vectors");

      geo.m_planes[sensorId] = Plane::fromDirections(unitU, unitV, offset);
    } else {
      auto rotX = cs.get<double>("rotation_x");
      auto rotY = cs.get<double>("rotation_y");
      auto rotZ = cs.get<double>("rotation_z");
      auto offX = cs.get<double>("offset_x");
      auto offY = cs.get<double>("offset_y");
      auto offZ = cs.get<double>("offset_z");
      geo.m_planes[sensorId] =
          Plane::fromAnglesZYX(rotZ, rotY, rotX, {offX, offY, offZ});
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
    Vector3 off = ip.second.offset;

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
  auto& plane = m_planes.at(sensorId);
  // TODO jacobian from dalpha,dbeta,dgamma to alpha,beta,gamma
  plane = plane.correctedGlobal(delta);
  m_covs[sensorId] = cov;
}

void Mechanics::Geometry::correctLocal(Index sensorId,
                                       const Vector6& delta,
                                       const SymMatrix6& cov)
{
  auto& plane = m_planes.at(sensorId);
  // Jacobian from local corrections to geometry parameters
  // TODO 2016-11-28 msmk: jacobian from dalpha,dbeta,dgamma to alpha,beta,gamma
  Matrix6 jac;
  jac.Place_at(plane.rotation, 0, 0);
  jac(3, 3) = 1;
  jac(4, 4) = 1;
  jac(5, 5) = 1;

  plane = plane.correctedLocal(delta);
  m_covs[sensorId] = Similarity(jac, cov);
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
       << prefix << "  offset: [" << ip.second.offset << "]\n"
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
    return geo.getPlane(id0).offset[2] < geo.getPlane(id1).offset[2];
  });
  return sorted;
}
