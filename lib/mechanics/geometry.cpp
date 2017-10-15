#include "geometry.h"

#include <cassert>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <TDecompSVD.h>
#include <TMatrix.h>

#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Geometry)

Mechanics::Plane Mechanics::Plane::fromAnglesZYX(double rotZ,
                                                 double rotY,
                                                 double rotX,
                                                 const Vector3& offset)
{
  Plane p;
  Rotation3D rot(RotationZYX(rotZ, rotY, rotX));
  rot.GetRotationMatrix(p.rotation);
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

Vector6 Mechanics::Plane::asParams() const
{
  Vector6 params;
  params[0] = offset[0];
  params[1] = offset[1];
  params[2] = offset[2];
  // decompose rotation matrix into zyx angles
  Rotation3D rot;
  rot.SetRotationMatrix(rotation);
  RotationZYX zyx(rot);
  params[3] = zyx.Psi();   // alpha
  params[4] = zyx.Theta(); // beta
  params[5] = zyx.Phi();   // gamma
  return params;
}

Vector3 Mechanics::Plane::toLocal(const Vector3& xyz) const
{
  return Transpose(rotation) * (xyz - offset);
}

Vector3 Mechanics::Plane::toLocal(const XYZPoint& xyz) const
{
  return Transpose(rotation) * (Vector3(xyz.x(), xyz.y(), xyz.z()) - offset);
}

Vector3 Mechanics::Plane::toGlobal(const Vector2& uv) const
{
  return offset + rotation.Sub<Matrix32>(0, 0) * uv;
}

Vector3 Mechanics::Plane::toGlobal(const XYPoint& uv) const
{
  return offset + rotation.Sub<Matrix32>(0, 0) * Vector2(uv.x(), uv.y());
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
  m_planes.at(sensorId).offset += Vector3(dx, dy, dz);
}

void Mechanics::Geometry::correctLocal(Index sensorId,
                                       const Vector6& delta,
                                       const SymMatrix6& cov)
{
  auto& plane = m_planes.at(sensorId);

  // Jacobian from local corrections to global geometry parameters
  // TODO 2016-11-28 msmk: jacobian from local rotU,... to alpha, beta, gamma
  Matrix6 jac;
  jac.Place_at(plane.rotation, 0, 0);
  jac(3, 3) = 1;
  jac(4, 4) = 1;
  jac(5, 5) = 1;
  m_covs[sensorId] = ROOT::Math::Similarity(jac, cov);

  // small angle approximation rotation matrix
  Matrix3 deltaQ;
  deltaQ(0, 0) = 1;
  deltaQ(0, 1) = -delta[5];
  deltaQ(0, 2) = -delta[4];
  deltaQ(1, 0) = delta[5];
  deltaQ(1, 1) = 1;
  deltaQ(1, 2) = -delta[3];
  deltaQ(2, 0) = delta[4];
  deltaQ(2, 1) = delta[3];
  deltaQ(2, 2) = 1;

  plane.rotation *= deltaQ;
  plane.offset += plane.rotation * delta.Sub<Vector3>(0);

  // due to the small angle approximation the combined rotation matrix might
  // be non-orthogonal (if only by small amounts). replace it by the closest
  // orthogonal matrix. see also:
  // https://en.wikipedia.org/wiki/Orthogonal_matrix#Nearest_orthogonal_matrix

  // Ahh ROOT, we have to use yet another linear algebra/ geometry package.
  // GenVector, SMatrix, and TMatrix; none of which like to talk to each other.

  TMatrixD M(3, 3);
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      M(i, j) = plane.rotation(i, j);
  TDecompSVD svd(M);
  if (!svd.Decompose())
    FAIL("sensor", sensorId, " rotation matrix decomposition failed.");

  DEBUG("sensor", sensorId, " updated matrix:");
  DEBUG("  unit u: [", plane.unitU(), "]");
  DEBUG("  unit v: [", plane.unitV(), "]");
  DEBUG("  unit w: [", plane.unitNormal(), "]");
  DEBUG("  singular values: [", svd.GetSig()[0], ", ", svd.GetSig()[1], ", ",
        svd.GetSig()[2], "]");
  double d1, d2;
  svd.Det(d1, d2);
  DEBUG("  determinant: ", d1 * std::pow(2, d2), " = (", d1, ")*2^(", d2, ")");

  // compute closest orthogonal matrix by setting singular values to 1
  TMatrixD U = svd.GetU();
  TMatrixD V = svd.GetV();
  TMatrixD Q = U * V.T();
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      plane.rotation(i, j) = Q(i, j);

  DEBUG("sensor", sensorId, " nearest rotation matrix:");
  DEBUG("  unit u: [", plane.unitU(), "]");
  DEBUG("  unit v: [", plane.unitV(), "]");
  DEBUG("  unit w: [", plane.unitNormal(), "]");
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
