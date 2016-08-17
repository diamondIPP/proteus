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

Mechanics::Alignment::Alignment(const char* fileName, Device* device)
    : m_fileName(fileName)
    , m_device(device)
{
  assert(device && "Alignment: can't initialize with a null device");
}

void Mechanics::Alignment::readFile(const std::string& path)
{
  ConfigParser config(path.c_str());
  parse(config);

  m_device->setBeamSlopeX(m_beamSlopeX);
  m_device->setBeamSlopeY(m_beamSlopeY);
  m_device->setSyncRatio(m_syncRatio);
  for (auto it = m_geo.cbegin(); it != m_geo.cend(); ++it) {
    unsigned int sensorId = it->first;
    const GeoParams& params = it->second;

    if (!(sensorId < m_device->getNumSensors()))
      throw std::runtime_error("Sensor id exceeds number of sensors");

    Sensor* sensor = m_device->getSensor(it->first);
    sensor->setOffX(params.offsetX);
    sensor->setOffY(params.offsetY);
    sensor->setOffZ(params.offsetZ);
    sensor->setRotX(params.rotationX);
    sensor->setRotY(params.rotationY);
    sensor->setRotZ(params.rotationZ);
  }
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

void Mechanics::Alignment::writeFile(const std::string& path)
{
  std::ofstream file(path);

  if (!file.is_open()) {
    std::string err = "[Alignment::writeFile] ERROR unable to open '" +
                      m_fileName + "' for writting";
    throw err.c_str();
  }

  file << std::setprecision(9);
  file << std::fixed;

  for (unsigned int nsens = 0; nsens < m_device->getNumSensors(); nsens++) {
    Sensor* sensor = m_device->getSensor(nsens);
    file << "[Sensor " << nsens << "]" << endl;
    file << "offset x   : " << sensor->getOffX() << endl;
    file << "offset y   : " << sensor->getOffY() << endl;
    file << "offset z   : " << sensor->getOffZ() << endl;
    file << "rotation x : " << sensor->getRotX() << endl;
    file << "rotation y : " << sensor->getRotY() << endl;
    file << "rotation z : " << sensor->getRotZ() << endl;
    file << "[End Sensor]\n" << endl;
  }

  file << "[Device]" << endl;
  file << "slope x    : " << m_device->getBeamSlopeX() << endl;
  file << "slope y    : " << m_device->getBeamSlopeY() << endl;
  if (m_device->getSyncRatio() > 0)
    file << "sync ratio : " << m_device->getSyncRatio() << endl;
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
