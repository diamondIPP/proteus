#include "alignment.h"

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

// ensure parameters have sensible defaults
Mechanics::Alignment::Alignment::PlaneParams()
    : offsetX(0)
    , offsetY(0)
    , offsetZ(0)
    , rotationX(0)
    , rotationY(0)
    , rotationZ(0)
{
}

Mechanics::Alignment::Alignment()
    : m_beamSlopeX(0), m_beamSlopeY(0), m_syncRatio(1)
{
}

Mechanics::Alignment Mechanics::Alignment::fromFile(const std::string& path)
{
  Alignment alignment;

  if (Utils::Config::pathExtension(path) == "toml") {
    alignment = fromConfig(Utils::Config::readConfig(path));
  } else {
    // fall-back to old format
    alignment = fromConfig(ConfigParser(path.c_str()));
  }
  INFO("read alignment from '", path, "'");
  return alignment;
}

void Mechanics::Alignment::writeFile(const std::string& path) const
{
  Utils::Config::writeConfig(toConfig(), path);
  INFO("wrote alignment to '", path, "'");
}

Mechanics::Alignment
Mechanics::Alignment::fromConfig(const ConfigParser& config)
{
  Alignment alignment;

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
        throw std::runtime_error("Alignment: failed to parse row");
      continue;
    }

    if (!row->header.compare("End Sensor"))
      continue;

    // header format should be "Sensor <#sensor_id>"
    if ((row->header.size() <= 7) ||
        row->header.substr(0, 7).compare("Sensor "))
      throw std::runtime_error("Alignment: found an invalid header");

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
      throw std::runtime_error("Alignment: failed to parse row");
  }

  return alignment;
}

Mechanics::Alignment Mechanics::Alignment::fromConfig(const toml::Value& cfg)
{
  Alignment alignment;

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

toml::Value Mechanics::Alignment::toConfig() const
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

void Mechanics::Alignment::setOffset(Index sensorId, const XYZPoint& offset)
{
  setOffset(sensorId, offset.x(), offset.y(), offset.z());
}

void Mechanics::Alignment::setOffset(Index sensorId,
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

void Mechanics::Alignment::setRotationAngles(Index sensorId,
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

void Mechanics::Alignment::correctGlobalOffset(Index sensorId,
                                               double dx,
                                               double dy,
                                               double dz)
{
  auto& params = m_params.at(sensorId);
  params.offsetX += dx;
  params.offsetY += dy;
  params.offsetZ += dz;
}

void Mechanics::Alignment::correctRotationAngles(Index sensorId,
                                                 double dalpha,
                                                 double dbeta,
                                                 double dgamma)
{
  auto& params = m_params.at(sensorId);
  params.rotationX += dalpha;
  params.rotationY += dbeta;
  params.rotationZ += dgamma;
}

void Mechanics::Alignment::correctLocal(Index sensorId, const Vector6& delta)
{
  auto& params = m_params.at(sensorId);

  // construct new local to global transformation with corrections
  Translation3D corrOff(delta[0], delta[1], delta[2]);
  // clang-format off
  Rotation3D corrRot(        1,  delta[3], -delta[4],
                     -delta[3],         1,  delta[5],
                      delta[4], -delta[5],         1);
  // clang-format on
  Transform3D l2g = getLocalToGlobal(sensorId) * corrRot * corrOff;
  // convert transformation back into geometry parameters
  l2g.Translation().GetComponents(params.offsetX, params.offsetY,
                                  params.offsetZ);
  RotationZYX rot;
  l2g.GetRotation(rot);
  rot.GetComponents(params.rotationZ, params.rotationY, params.rotationX);
}

bool Mechanics::Alignment::hasAlignment(Index sensorId) const
{
  return (0 < m_params.count(sensorId));
}

Transform3D Mechanics::Alignment::getLocalToGlobal(Index sensorId) const
{
  auto it = m_params.find(sensorId);
  if (it == m_params.cend())
    return Transform3D();

  const PlaneParams& params = it->second;

  XYZVector off(params.offsetX, params.offsetY, params.offsetZ);
  RotationZYX rot(params.rotationZ, params.rotationY, params.rotationX);
  return Transform3D(rot, off);
}

Vector6 Mechanics::Alignment::getParams(Index sensorId) const
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

void Mechanics::Alignment::setBeamSlope(double slopeX, double slopeY)
{
  m_beamSlopeX = slopeX;
  m_beamSlopeY = slopeY;
}

XYZVector Mechanics::Alignment::beamDirection() const
{
  return XYZVector(m_beamSlopeX, m_beamSlopeY, 1);
}

void Mechanics::Alignment::print(std::ostream& os,
                                 const std::string& prefix) const
{
  const double RAD2DEG = 180 / M_PI;

  os << prefix << "beam:\n"
     << prefix << "  slope X: " << m_beamSlopeX << '\n'
     << prefix << "  slope Y: " << m_beamSlopeY << '\n';

  for (auto ip = m_geo.begin(); ip != m_geo.end(); ++ip) {
    Index sensorId = ip->first;
    const GeoParams& p = ip->second;
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
