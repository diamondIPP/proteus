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
    geo.m_planes[sensorId] =
        Plane::fromAnglesZYX(rotZ, rotY, rotX, {offX, offY, offZ});
  }
  return geo;
}

toml::Value Mechanics::Geometry::toConfig() const
{
  toml::Value cfg;

  cfg["beam"]["slope_x"] = m_beamSlopeX;
  cfg["beam"]["slope_y"] = m_beamSlopeY;

  cfg["sensors"] = toml::Array();
  for (const auto& ip : m_planes) {
    int id = static_cast<int>(ip.first);
    Vector6 params = ip.second.asParams();

    toml::Value cfgSensor;
    cfgSensor["id"] = id;
    cfgSensor["offset_x"] = params[0];
    cfgSensor["offset_y"] = params[1];
    cfgSensor["offset_z"] = params[2];
    cfgSensor["rotation_x"] = params[3];
    cfgSensor["rotation_y"] = params[4];
    cfgSensor["rotation_z"] = params[5];
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
  // TODO 2017-05-31 msmk: rectify the rotation matrix
}

Transform3D Mechanics::Geometry::getLocalToGlobal(Index sensorId) const
{
  return m_planes.at(sensorId).asTransform3D();
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

XYZVector Mechanics::Geometry::beamDirection() const
{
  return XYZVector(m_beamSlopeX, m_beamSlopeY, 1);
}

void Mechanics::Geometry::print(std::ostream& os,
                                const std::string& prefix) const
{
  os << prefix << "beam:\n"
     << prefix << "  slope x: " << m_beamSlopeX << '\n'
     << prefix << "  slope y: " << m_beamSlopeY << '\n';
  for (const auto& ip : m_planes) {
    os << prefix << "sensor " << ip.first << ":\n"
       << prefix << "  offset: [" << ip.second.offset << "]\n"
       << prefix << "  unit u: [" << ip.second.unitU() << "]\n"
       << prefix << "  unit v: [" << ip.second.unitV() << "]\n"
       << prefix << "  unit w: [" << ip.second.unitNormal() << "]\n";
  }
  os.flush();
}
