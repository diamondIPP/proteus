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
    : _fileName(fileName)
    , _device(device)
{
  assert(device && "Alignment: can't initialize with a null device");
}

void Mechanics::Alignment::readFile(const std::string& path)
{
  ConfigParser config(path.c_str());
  parse(config);

  _device->setBeamSlopeX(_beamSlopeX);
  _device->setBeamSlopeY(_beamSlopeY);
  _device->setSyncRatio(_syncRatio);
  for (auto it = _geo.cbegin(); it != _geo.cend(); ++it) {
    unsigned int sensor_id = it->first;
    const Geometry& geo = it->second;

    if (!(sensor_id < _device->getNumSensors()))
      throw std::runtime_error("Sensor id exceeds number of sensors");

    Sensor* sensor = _device->getSensor(it->first);
    sensor->setOffX(geo.offsetX);
    sensor->setOffY(geo.offsetY);
    sensor->setOffZ(geo.offsetZ);
    sensor->setRotX(geo.rotationX);
    sensor->setRotY(geo.rotationY);
    sensor->setRotZ(geo.rotationZ);
  }
}

void Mechanics::Alignment::parse(const ConfigParser& config)
{
  _geo.clear();
  _beamSlopeX = 0;
  _beamSlopeY = 0;
  _syncRatio = 0;

  for (unsigned int nrow = 0; nrow < config.getNumRows(); nrow++) {
    const ConfigParser::Row* row = config.getRow(nrow);

    // No action to take when encoutering a header
    if (row->isHeader)
      continue;

    if (!row->header.compare("Device")) {
      const double value = ConfigParser::valueToNumerical(row->value);
      if (!row->key.compare("slope x"))
        _beamSlopeX = value;
      else if (!row->key.compare("slope y"))
        _beamSlopeY = value;
      else if (!row->key.compare("sync ratio"))
        _syncRatio = value;
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
      _geo[isens].offsetX = value;
    else if (!row->key.compare("offset y"))
      _geo[isens].offsetY = value;
    else if (!row->key.compare("offset z"))
      _geo[isens].offsetZ = value;
    else if (!row->key.compare("rotation x"))
      _geo[isens].rotationX = value;
    else if (!row->key.compare("rotation y"))
      _geo[isens].rotationY = value;
    else if (!row->key.compare("rotation z"))
      _geo[isens].rotationZ = value;
    else
      throw std::runtime_error("Alignment: can't parse config row");
  }
}

void Mechanics::Alignment::writeFile(const std::string& path)
{
  std::ofstream file(path);

  if (!file.is_open()) {
    std::string err = "[Alignment::writeFile] ERROR unable to open '" +
                      std::string(_fileName) + "' for writting";
    throw err.c_str();
  }

  file << std::setprecision(9);
  file << std::fixed;

  for (unsigned int nsens = 0; nsens < _device->getNumSensors(); nsens++) {
    Sensor* sensor = _device->getSensor(nsens);
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
  file << "slope x    : " << _device->getBeamSlopeX() << endl;
  file << "slope y    : " << _device->getBeamSlopeY() << endl;
  if (_device->getSyncRatio() > 0)
    file << "sync ratio : " << _device->getSyncRatio() << endl;
  file << "[End Device]\n" << endl;

  file.close();

  std::cout << "\nAlignment file '" << _fileName << "' created OK\n"
            << std::endl;
}

bool Mechanics::Alignment::hasAlignment(Index sensorId) const
{
  return (0 < _geo.count(sensorId));
}

Transform3 Mechanics::Alignment::getLocalToGlobal(Index sensorId) const
{
  auto it = _geo.find(sensorId);
  if (it == _geo.cend())
    return Transform3();

  const Geometry& g = it->second;

  Vector3 off(g.offsetX, g.offsetY, g.offsetZ);
  RotationZYX rot(g.rotationZ, g.rotationY, g.rotationX);
  return Transform3(rot, off);
}
