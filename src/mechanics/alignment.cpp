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

using Utils::logger;

Mechanics::Alignment::Alignment()
    : m_beamSlopeX(0)
    , m_beamSlopeY(0)
    , m_syncRatio(1)
{
}

Mechanics::Alignment Mechanics::Alignment::fromFile(const std::string& path)
{
  ConfigParser config(path.c_str());
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
      alignment.m_geo[isens].offsetX = value;
    else if (!row->key.compare("offset y"))
      alignment.m_geo[isens].offsetY = value;
    else if (!row->key.compare("offset z"))
      alignment.m_geo[isens].offsetZ = value;
    else if (!row->key.compare("rotation x"))
      alignment.m_geo[isens].rotationX = value;
    else if (!row->key.compare("rotation y"))
      alignment.m_geo[isens].rotationY = value;
    else if (!row->key.compare("rotation z"))
      alignment.m_geo[isens].rotationZ = value;
    else
      throw std::runtime_error("Alignment: failed to parse row");
  }

  INFO("read alignment from '", path, "'\n");
  return alignment;
}

void Mechanics::Alignment::writeFile(const std::string& path) const
{
  using std::endl;

  std::ofstream file(path);

  if (!file.is_open())
    throw std::runtime_error("Alignment: failed to open file '" + path +
                             "' to write");

  file << std::setprecision(9);
  file << std::fixed;

  for (auto it = m_geo.begin(); it != m_geo.end(); ++it) {
    Index sensorId = it->first;
    const GeoParams& params = it->second;

    file << "[Sensor " << sensorId << "]" << endl;
    file << "offset x   : " << params.offsetX << endl;
    file << "offset y   : " << params.offsetY << endl;
    file << "offset z   : " << params.offsetZ << endl;
    file << "rotation x : " << params.rotationX << endl;
    file << "rotation y : " << params.rotationY << endl;
    file << "rotation z : " << params.rotationZ << endl;
    file << "[End Sensor]\n" << endl;
  }

  file << "[Device]" << endl;
  file << "slope x    : " << m_beamSlopeX << endl;
  file << "slope y    : " << m_beamSlopeY << endl;
  file << "sync ratio : " << m_syncRatio << endl;
  file << "[End Device]\n" << endl;

  file.close();

  INFO("wrote alignment to '", path, "'\n");
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
    alignment.setOffset(is->get<int>("id"),
                        is->get<double>("offset_x"),
                        is->get<double>("offset_y"),
                        is->get<double>("offset_z"));
    alignment.setRotationAngles(is->get<int>("id"),
                                is->get<double>("rotation_x"),
                                is->get<double>("rotation_y"),
                                is->get<double>("rotation_z"));
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
    for (auto ig = m_geo.begin(); ig != m_geo.end(); ++ig) {
        int id = static_cast<int>(ig->first);
        const GeoParams& params = ig->second;

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

bool Mechanics::Alignment::hasAlignment(Index sensorId) const
{
  return (0 < m_geo.count(sensorId));
}

Transform3D Mechanics::Alignment::getLocalToGlobal(Index sensorId) const
{
  auto it = m_geo.find(sensorId);
  if (it == m_geo.cend())
    return Transform3D();

  const GeoParams& params = it->second;

  XYZVector off(params.offsetX, params.offsetY, params.offsetZ);
  RotationZYX rot(params.rotationZ, params.rotationY, params.rotationX);
  return Transform3D(rot, off);
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
  // will automatically create a missing GeoParams
  auto& params = m_geo[sensorId];
  params.offsetX = x;
  params.offsetY = y;
  params.offsetZ = z;
}

void Mechanics::Alignment::setRotationAngles(Index sensorId,
                                             double rotX,
                                             double rotY,
                                             double rotZ)
{
  // will automatically create a missing GeoParams
  auto& params = m_geo[sensorId];
  params.rotationX = rotX;
  params.rotationY = rotY;
  params.rotationZ = rotZ;
}

void Mechanics::Alignment::correctOffset(Index sensorId,
                                         double dx,
                                         double dy,
                                         double dz)
{
  auto& params = m_geo[sensorId];
  params.offsetX += dx;
  params.offsetY += dy;
  params.offsetZ += dz;
}

void Mechanics::Alignment::correctRotationAngles(Index sensorId,
                                                 double dalpha,
                                                 double dbeta,
                                                 double dgamma)
{
  auto& params = m_geo[sensorId];
  params.rotationX += dalpha;
  params.rotationY += dbeta;
  params.rotationZ += dgamma;
}

XYZVector Mechanics::Alignment::beamDirection() const
{
  return XYZVector(m_beamSlopeX, m_beamSlopeY, 1);
}

void Mechanics::Alignment::setBeamSlope(double slopeX, double slopeY)
{
  m_beamSlopeX = slopeX;
  m_beamSlopeY = slopeY;
}

void Mechanics::Alignment::print(std::ostream& os,
                                 const std::string& prefix) const
{
  os << prefix << "beam:\n"
     << prefix << "  slope X: " << m_beamSlopeX << '\n'
     << prefix << "  slope Y: " << m_beamSlopeY << '\n';

  auto ip = m_geo.begin();
  for (; ip != m_geo.end(); ++ip) {
    Index i = ip->first;
    const GeoParams& p = ip->second;

    os << prefix << "sensor " << i << ":\n";
    os << prefix << "  offset X: " << p.offsetX << '\n'
       << prefix << "  offset Y: " << p.offsetY << '\n'
       << prefix << "  offset Z: " << p.offsetZ << '\n';
    os << prefix << "  rotation X: " << p.rotationX << '\n'
       << prefix << "  rotation Y: " << p.rotationY << '\n'
       << prefix << "  rotation Z: " << p.rotationZ << '\n';
  }
  os.flush();
}
