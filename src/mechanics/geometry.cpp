#include "geometry.h"

#include <cassert>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "utils/configparser.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

// ensure geometry parameters have sensible defaults
Mechanics::Geometry::PlaneParams::PlaneParams()
    : offsetX(0)
    , offsetY(0)
    , offsetZ(0)
    , rotationX(0)
    , rotationY(0)
    , rotationZ(0)
{
  // covariance should be zero by default constructor
}

Mechanics::Geometry::Geometry()
    : m_beamSlopeX(0), m_beamSlopeY(0), m_syncRatio(1)
{
}

Mechanics::Geometry Mechanics::Geometry::fromFile(const std::string& path)
{
  Geometry alignment;

  if (Utils::Config::pathExtension(path) == "toml") {
    alignment = fromConfig(Utils::Config::readConfig(path));
  } else {
    // fall-back to old format
    alignment = fromConfig(ConfigParser(path.c_str()));
  }
  INFO("read alignment from '", path, "'");
  return alignment;
}

void Mechanics::Geometry::writeFile(const std::string& path) const
{
  Utils::Config::writeConfig(toConfig(), path);
  INFO("wrote alignment to '", path, "'");
}

Mechanics::Geometry Mechanics::Geometry::fromConfig(const ConfigParser& config)
{
  Geometry alignment;

  for (unsigned int nrow = 0; nrow < config.getNumRows(); nrow++) {
    const ConfigParser::Row* row = config.getRow(nrow);

    // No action to take when encoutering a header
    if (row->isHeader)
      continue;

    if (!row->header.compare("Device")) {
      const double value = ConfigParser::valueToNumerical(row->value);
      if (!row->key.compare("slope x"))
        alignment.m_beamSlopeX = value;
      else if (!row->key.compare("slope y"))
        alignment.m_beamSlopeY = value;
      else if (!row->key.compare("sync ratio"))
        alignment.m_syncRatio = value;
      else
        throw std::runtime_error("Geometry: failed to parse row");
      continue;
    }

    if (!row->header.compare("End Sensor"))
      continue;

    // header format should be "Sensor <#sensor_id>"
    if ((row->header.size() <= 7) ||
        row->header.substr(0, 7).compare("Sensor "))
      throw std::runtime_error("Geometry: found an invalid header");

    unsigned int isens = std::stoi(row->header.substr(7));
    const double value = ConfigParser::valueToNumerical(row->value);
    if (!row->key.compare("offset x"))
      alignment.m_params[isens].offsetX = value;
    else if (!row->key.compare("offset y"))
      alignment.m_params[isens].offsetY = value;
    else if (!row->key.compare("offset z"))
      alignment.m_params[isens].offsetZ = value;
    else if (!row->key.compare("rotation x"))
      alignment.m_params[isens].rotationX = value;
    else if (!row->key.compare("rotation y"))
      alignment.m_params[isens].rotationY = value;
    else if (!row->key.compare("rotation z"))
      alignment.m_params[isens].rotationZ = value;
    else
      throw std::runtime_error("Geometry: failed to parse row");
  }

  return alignment;
}

Mechanics::Geometry Mechanics::Geometry::fromConfig(const toml::Value& cfg)
{
  Geometry alignment;

  alignment.setBeamSlope(cfg.get<double>("beam.slope_x"),
                         cfg.get<double>("beam.slope_y"));
  if (cfg.has("timing.sync_ratio"))
    alignment.setSyncRatio(cfg.get<double>("timing.sync_ratio"));

  auto sensors = cfg.get<toml::Array>("sensors");
  for (auto is = sensors.begin(); is != sensors.end(); ++is) {
    alignment.setOffset(is->get<int>("id"), is->get<double>("offset_x"),
                        is->get<double>("offset_y"),
                        is->get<double>("offset_z"));
    alignment.setRotationAngles(
        is->get<int>("id"), is->get<double>("rotation_x"),
        is->get<double>("rotation_y"), is->get<double>("rotation_z"));
  }
  return alignment;
}

toml::Value Mechanics::Geometry::toConfig() const
{
  toml::Value cfg;

  cfg["beam"]["slope_x"] = m_beamSlopeX;
  cfg["beam"]["slope_y"] = m_beamSlopeY;
  cfg["timing"]["sync_ratio"] = m_syncRatio;

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

void Mechanics::Geometry::setOffset(Index sensorId, const XYZPoint& offset)
{
  setOffset(sensorId, offset.x(), offset.y(), offset.z());
}

void Mechanics::Geometry::setOffset(Index sensorId,
                                    double x,
                                    double y,
                                    double z)
{
  // will automatically create a missing PlaneParams
  auto& params = m_params[sensorId];
  params.offsetX = x;
  params.offsetY = y;
  params.offsetZ = z;
}

void Mechanics::Geometry::setRotationAngles(Index sensorId,
                                            double rotX,
                                            double rotY,
                                            double rotZ)
{
  // will automatically create a missing PlaneParams
  auto& params = m_params[sensorId];
  params.rotationX = rotX;
  params.rotationY = rotY;
  params.rotationZ = rotZ;
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

void Mechanics::Geometry::correctRotationAngles(Index sensorId,
                                                double dalpha,
                                                double dbeta,
                                                double dgamma)
{
  auto& params = m_params.at(sensorId);
  params.rotationX += dalpha;
  params.rotationY += dbeta;
  params.rotationZ += dgamma;
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
  params.cov += ROOT::Math::Similarity(jac, cov);
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

const SymMatrix6& Mechanics::Geometry::getParamsCov(Index sensorId) const
{
  return m_params.at(sensorId).cov;
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