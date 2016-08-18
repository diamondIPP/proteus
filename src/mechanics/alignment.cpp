#include "mechanics/alignment.h"

#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "configparser.h"
#include "mechanics/device.h"
#include "mechanics/sensor.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

using std::fstream;
using std::cout;
using std::endl;

Mechanics::Alignment::Alignment(const std::string& fileName)
    : m_fileName(fileName)
{
}

void Mechanics::Alignment::readFile(const std::string& path)
{
  parse(ConfigParser(path.c_str()));
}

void Mechanics::Alignment::parse(const ConfigParser& config)
{
  m_geo.clear();
  m_beamSlopeX = 0;
  m_beamSlopeY = 0;
  m_syncRatio = 0;

  for (unsigned int nrow = 0; nrow < config.getNumRows(); nrow++) {
    const ConfigParser::Row* row = config.getRow(nrow);

    // No action to take when encoutering a header
    if (row->isHeader)
      continue;

    if (!row->header.compare("Device")) {
      const double value = ConfigParser::valueToNumerical(row->value);
      if (!row->key.compare("slope x"))
        m_beamSlopeX = value;
      else if (!row->key.compare("slope y"))
        m_beamSlopeY = value;
      else if (!row->key.compare("sync ratio"))
        m_syncRatio = value;
      else
        throw std::runtime_error("Alignment: can't parse config row");
      continue;
    }

    if (!row->header.compare("End Sensor"))
      continue;

    // header format should be "Sensor <#sensor_id>"
    if ((row->header.size() <= 7) ||
        row->header.substr(0, 7).compare("Sensor "))
      throw std::runtime_error("AlignmentFile: parsed an incorrect header");

    unsigned int isens = std::stoi(row->header.substr(7));
    const double value = ConfigParser::valueToNumerical(row->value);
    if (!row->key.compare("offset x"))
      m_geo[isens].offsetX = value;
    else if (!row->key.compare("offset y"))
      m_geo[isens].offsetY = value;
    else if (!row->key.compare("offset z"))
      m_geo[isens].offsetZ = value;
    else if (!row->key.compare("rotation x"))
      m_geo[isens].rotationX = value;
    else if (!row->key.compare("rotation y"))
      m_geo[isens].rotationY = value;
    else if (!row->key.compare("rotation z"))
      m_geo[isens].rotationZ = value;
    else
      throw std::runtime_error("Alignment: can't parse config row");
  }
}

void Mechanics::Alignment::writeFile(const std::string& path) const
{
  std::ofstream file(path);

  if (!file.is_open()) {
    std::string err = "[Alignment::writeFile] ERROR unable to open '" +
                      m_fileName + "' for writting";
    throw err.c_str();
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

  std::cout << "\nAlignment file '" << m_fileName << "' created OK\n"
            << std::endl;
}

bool Mechanics::Alignment::hasAlignment(Index sensorId) const
{
  return (0 < m_geo.count(sensorId));
}

Transform3 Mechanics::Alignment::getLocalToGlobal(Index sensorId) const
{
  auto it = m_geo.find(sensorId);
  if (it == m_geo.cend())
    return Transform3();

  const GeoParams& params = it->second;

  Vector3 off(params.offsetX, params.offsetY, params.offsetZ);
  RotationZYX rot(params.rotationZ, params.rotationY, params.rotationX);
  return Transform3(rot, off);
}

void Mechanics::Alignment::setOffset(Index sensorId, const Point3& offset)
{
  // will automatically create a missing GeoParams
  auto& params = m_geo[sensorId];
  params.offsetX = offset.x();
  params.offsetY = offset.y();
  params.offsetZ = offset.z();
}

void Mechanics::Alignment::setRotationAngles(Index sensorId, double rotX, double rotY, double rotZ)
{
  // will automatically create a missing GeoParams
  auto& params = m_geo[sensorId];
  params.rotationX = rotX;
  params.rotationY = rotY;
  params.rotationZ = rotZ;
}

Vector3 Mechanics::Alignment::beamDirection() const
{
  double f = 1 / std::hypot(1, std::hypot(m_beamSlopeX, m_beamSlopeY));
  return Vector3(f * m_beamSlopeX, f * m_beamSlopeY, f);
}
