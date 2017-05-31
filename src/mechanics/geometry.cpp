#include "geometry.h"

#include <cassert>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

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

Transform3D Mechanics::Plane::asTransform3D() const
{
  Rotation3D r;
  r.SetRotationMatrix(rotation);
  return Transform3D(r, Translation3D(offset[0], offset[1], offset[2]));
}

// ensure geometry parameters have sensible defaults
Mechanics::Geometry::PlaneParams::PlaneParams()
    : offsetX(0)
    , offsetY(0)
    , offsetZ(0)
    , rotationX(0)
    , rotationY(0)
    , rotationZ(0)
{
}

Mechanics::Geometry::Geometry() : m_beamSlopeX(0), m_beamSlopeY(0) {}

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

  geo.setBeamSlope(cfg.get<double>("beam.slope_x"),
                   cfg.get<double>("beam.slope_y"));

  auto sensors = cfg.get<toml::Array>("sensors");
  for (const auto& cs : sensors) {
    auto sensorId = cs.get<int>("id");
    auto rotX = cs.get<double>("rotation_x");
    auto rotY = cs.get<double>("rotation_y");
    auto rotZ = cs.get<double>("rotation_z");
    auto offX = cs.get<double>("offset_x");
    auto offY = cs.get<double>("offset_y");
    auto offZ = cs.get<double>("offset_z");
    // missing parameter object gets automatically created
    auto& params = geo.m_params[sensorId];
    params.offsetX = offX;
    params.offsetY = offY;
    params.offsetZ = offZ;
    params.rotationX = rotX;
    params.rotationY = rotY;
    params.rotationZ = rotZ;
  }
  return geo;
}

toml::Value Mechanics::Geometry::toConfig() const
{
  toml::Value cfg;

  cfg["beam"]["slope_x"] = m_beamSlopeX;
  cfg["beam"]["slope_y"] = m_beamSlopeY;

  cfg["sensors"] = toml::Array();
  for (auto ig = m_params.begin(); ig != m_params.end(); ++ig) {
    int id = static_cast<int>(ig->first);
    const PlaneParams& params = ig->second;

    toml::Value cfgSensor;
    cfgSensor["id"] = id;
    cfgSensor["offset_x"] = params.offsetX;
    cfgSensor["offset_y"] = params.offsetY;
    cfgSensor["offset_z"] = params.offsetZ;
    cfgSensor["rotation_x"] = params.rotationX;
    cfgSensor["rotation_y"] = params.rotationY;
    cfgSensor["rotation_z"] = params.rotationZ;
    cfg["sensors"].push(std::move(cfgSensor));
  }
  return cfg;
}

void Mechanics::Geometry::correctGlobalOffset(Index sensorId,
                                              double dx,
                                              double dy,
                                              double dz)
{
  auto& params = m_params.at(sensorId);
  params.offsetX += dx;
  params.offsetY += dy;
  params.offsetZ += dz;
}

void Mechanics::Geometry::correctLocal(Index sensorId,
                                       const Vector6& delta,
                                       const SymMatrix6& cov)
{
  auto& params = m_params.at(sensorId);

  // small angle approximation rotation matrix
  // clang-format off
  double coeffs[9] = {
           1, -delta[5], -delta[4],
    delta[5],         1, -delta[3],
    delta[4],  delta[3],         1};
  // clang-format on
  Matrix3 deltaQ(coeffs, 9);
  Matrix3 rot;
  Vector3 off;
  Transform3D old = getLocalToGlobal(sensorId);

  // construct new rotation matrix and offset
  old.Rotation().GetRotationMatrix(rot);
  old.Translation().GetComponents(off.begin(), off.end());
  rot *= deltaQ;
  off += rot * delta.Sub<Vector3>(0);

  // decompose transformation back into offsets and angles
  params.offsetX = off[0];
  params.offsetY = off[1];
  params.offsetZ = off[2];
  // ROOT WTF: multiple rotation matrix types that do not work together
  Rotation3D tmp;
  tmp.SetRotationMatrix(rot);
  RotationZYX(tmp).GetComponents(params.rotationZ, params.rotationY,
                                 params.rotationX);

  // Jacobian from local corrections to global geometry parameters
  // TODO 2016-11-28 msmk: jacobian from local rotU,... to alpha, beta, gamma
  Matrix6 jac;
  jac.Place_at(rot, 0, 0);
  jac(3, 3) = 1;
  jac(4, 4) = 1;
  jac(5, 5) = 1;
  m_covs[sensorId] = ROOT::Math::Similarity(jac, cov);
}

Transform3D Mechanics::Geometry::getLocalToGlobal(Index sensorId) const
{
  auto it = m_params.find(sensorId);
  if (it == m_params.cend())
    return Transform3D();

  const PlaneParams& params = it->second;

  XYZVector off(params.offsetX, params.offsetY, params.offsetZ);
  RotationZYX rot(params.rotationZ, params.rotationY, params.rotationX);
  return Transform3D(rot, off);
}

Vector6 Mechanics::Geometry::getParams(Index sensorId) const
{
  const PlaneParams& params = m_params.at(sensorId);
  Vector6 p;
  p[0] = params.offsetX;
  p[1] = params.offsetY;
  p[2] = params.offsetZ;
  p[3] = params.rotationX;
  p[4] = params.rotationY;
  p[5] = params.rotationZ;
  return p;
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

XYZVector Mechanics::Geometry::beamDirection() const
{
  return XYZVector(m_beamSlopeX, m_beamSlopeY, 1);
}

void Mechanics::Geometry::print(std::ostream& os,
                                const std::string& prefix) const
{
  const double RAD2DEG = 180 / M_PI;

  os << prefix << "beam:\n"
     << prefix << "  slope X: " << m_beamSlopeX << '\n'
     << prefix << "  slope Y: " << m_beamSlopeY << '\n';

  for (auto params = m_params.begin(); params != m_params.end(); ++params) {
    Index sensorId = params->first;
    const PlaneParams& p = params->second;
    Vector3 unitU, unitV, unitW;
    getLocalToGlobal(sensorId).Rotation().GetComponents(unitU, unitV, unitW);

    os << prefix << "sensor " << sensorId << ":\n";
    os << prefix << "  offset x: " << p.offsetX << '\n'
       << prefix << "  offset y: " << p.offsetY << '\n'
       << prefix << "  offset z: " << p.offsetZ << '\n';
    os << prefix << "  rotation x: " << p.rotationX * RAD2DEG << " deg\n"
       << prefix << "  rotation y: " << p.rotationY * RAD2DEG << " deg\n"
       << prefix << "  rotation z: " << p.rotationZ * RAD2DEG << " deg\n";
    os << prefix << "  unit vector u: [" << unitU << "]\n"
       << prefix << "  unit vector v: [" << unitV << "]\n"
       << prefix << "  unit vector w: [" << unitW << "]\n";
  }
  os.flush();
}
