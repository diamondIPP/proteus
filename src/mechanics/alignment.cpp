#include "alignment.h"

#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "configparser.h"
#include "utils/logger.h"

using Utils::logger;

Mechanics::Alignment Mechanics::Alignment::fromFile(const std::string& path)
{
  auto alignment = fromConfig(ConfigParser(path.c_str()));
  INFO("read alignment from '", path, "'\n");
  return alignment;
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
  return alignment;
}

Mechanics::Alignment::Alignment()
    : m_beamSlopeX(0)
    , m_beamSlopeY(0)
    , m_syncRatio(1)
{
}

void Mechanics::Alignment::writeFile(const std::string& path) const
{
  using std::endl;

  std::ofstream file(path);

  if (!file.is_open()) {
    std::string msg("Alignment: failed to open file '");
    msg += path;
    msg += '\'';
    throw std::runtime_error(msg);
  }

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
  double f = 1 / std::hypot(1, std::hypot(m_beamSlopeX, m_beamSlopeY));
  return XYZVector(f * m_beamSlopeX, f * m_beamSlopeY, f);
}

void Mechanics::Alignment::setBeamSlope(double slopeX, double slopeY)
{
  m_beamSlopeX = slopeX;
  m_beamSlopeY = slopeY;
}

void Mechanics::Alignment::correctBeamSlope(double dslopeX, double dslopeY)
{
  m_beamSlopeX += dslopeX;
  m_beamSlopeY += dslopeY;
}

void Mechanics::Alignment::print(std::ostream& os,
                                 const std::string& prefix) const
{
  os << prefix << "Alignment:\n";
  os << prefix << "  Beam:\n"
     << prefix << "    Slope X: " << m_beamSlopeX << '\n'
     << prefix << "    Slope Y: " << m_beamSlopeY << '\n';

  auto ip = m_geo.begin();
  for (; ip != m_geo.end(); ++ip) {
    Index i = ip->first;
    const GeoParams& p = ip->second;

    os << prefix << "  Sensor " << i << ":\n";
    os << prefix << "    Offset X: " << p.offsetX << '\n'
       << prefix << "    Offset Y: " << p.offsetY << '\n'
       << prefix << "    Offset Z: " << p.offsetZ << '\n';
    os << prefix << "    Rotation X: " << p.rotationX << '\n'
       << prefix << "    Rotation Y: " << p.rotationY << '\n'
       << prefix << "    Rotation Z: " << p.rotationZ << '\n';
  }
  os.flush();
}
