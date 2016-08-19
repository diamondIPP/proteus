#include "device.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string.h>
#include <string>
#include <vector>

#include "configparser.h"
#include "utils/definitions.h"

using std::cout;
using std::endl;

static void parseSensors(const ConfigParser& config,
                         Mechanics::Device& device,
                         Mechanics::Alignment& alignment)
{
  double offX = 0;
  double offY = 0;
  double offZ = 0;
  double rotX = 0;
  double rotY = 0;
  double rotZ = 0;
  unsigned int cols = 0;
  unsigned int rows = 0;
  double depth = 0;
  double pitchX = 0;
  double pitchY = 0;
  double xox0 = 0;
  std::string name = "";
  bool masked = false;
  bool digi = false;
  bool alignable = true;

  unsigned int sensorCounter = 0;

  for (unsigned int i = 0; i < config.getNumRows(); i++) {
    const ConfigParser::Row* row = config.getRow(i);

    if (row->isHeader && !row->header.compare("End Sensor")) {
      if (!masked) {
        if (cols <= 0 || rows <= 0 || pitchX <= 0 || pitchY <= 0)
          throw std::runtime_error(
              "Device: need cols, rows, pitchX and pitchY to make a sensor");

        if (name.empty())
          name = "Plane" + std::to_string(sensorCounter);

        Mechanics::Sensor sensor(name, cols, rows, pitchX, pitchY, depth, xox0);
        device.addSensor(sensor);
        alignment.setOffset(sensorCounter, offX, offY, offZ);
        alignment.setRotationAngles(sensorCounter, rotX, rotY, rotZ);
        sensorCounter++;
      } else {
        device.addMaskedSensor();
      }

      offX = 0;
      offY = 0;
      offZ = 0;
      rotX = 0;
      rotY = 0;
      rotZ = 0;
      cols = 0;
      rows = 0;
      depth = 0;
      pitchX = 0;
      pitchY = 0;
      xox0 = 0;
      name = "";
      masked = false;
      digi = false;
      alignable = true;

      continue;
    }

    if (row->isHeader)
      continue;

    if (row->header.compare("Sensor"))
      continue;

    if (!row->key.compare("offset x"))
      offX = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("offset y"))
      offY = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("offset z"))
      offZ = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("rotation x"))
      rotX = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("rotation y"))
      rotY = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("rotation z"))
      rotZ = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("cols"))
      cols = (int)ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("rows"))
      rows = (int)ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("depth"))
      depth = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("pitch x"))
      pitchX = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("pitch y"))
      pitchY = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("xox0"))
      xox0 = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("name"))
      name = row->value;
    else if (!row->key.compare("masked"))
      masked = ConfigParser::valueToLogical(row->value);
    else if (!row->key.compare("digital"))
      digi = ConfigParser::valueToLogical(row->value);
    else if (!row->key.compare("alignable"))
      alignable = ConfigParser::valueToLogical(row->value);
    else {
      std::string err("Device: can't parse line ");
      std::stringstream ss;
      ss << i;
      err += ss.str() + " with [key,value]=['" + row->key + "','" + row->value +
             "'] in cfg file '";
      err += std::string(config.getFilePath()) + "'";
      throw std::runtime_error(err);
    }
  }
}

static Mechanics::Device parseDevice(const ConfigParser& config)
{
  std::string name = "";
  std::string alignmentName = "";
  std::string noiseMaskName = "";
  std::string spaceUnit = "";
  std::string timeUnit = "";
  double clockRate = 0;
  double readOutWindow = 0;

  for (unsigned int i = 0; i < config.getNumRows(); i++) {
    const ConfigParser::Row* row = config.getRow(i);

    if (row->isHeader && !row->header.compare("End Device")) {
      Mechanics::Device device(
          name, clockRate, readOutWindow, spaceUnit, timeUnit);
      Mechanics::Alignment alignment;

      parseSensors(config, device, alignment);

      if (!alignmentName.empty()) {
        device.applyAlignment(Mechanics::Alignment::fromFile(alignmentName));
      } else {
        device.applyAlignment(alignment);
      }
      if (!noiseMaskName.empty())
        device.applyNoiseMask(Mechanics::NoiseMask::fromFile(noiseMaskName));
      return device;
    }

    if (row->isHeader)
      continue;

    if (row->header.compare("Device"))
      continue; // Skip non-device rows

    if (!row->key.compare("name"))
      name = row->value;
    else if (!row->key.compare("alignment"))
      alignmentName = row->value;
    else if (!row->key.compare("noise mask"))
      noiseMaskName = row->value;
    else if (!row->key.compare("clock"))
      clockRate = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("window"))
      readOutWindow = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("space unit"))
      spaceUnit = row->value;
    else if (!row->key.compare("time unit"))
      timeUnit = row->value;
    else
      throw std::runtime_error("Can't parse device row.");
  }

  // Control shouldn't arrive at this point
  throw std::runtime_error("No device was parsed.");
}

Mechanics::Device::Device(const std::string& name,
                          double clockRate,
                          unsigned int readoutWindow,
                          const std::string& spaceUnit,
                          const std::string& timeUnit)
    : m_name(name)
    , m_clockRate(clockRate)
    , m_readoutWindow(readoutWindow)
    , m_timeStart(0)
    , m_timeEnd(0)
    , m_syncRatio(0)
    , m_beamSlopeX(0)
    , m_beamSlopeY(0)
    , m_spaceUnit(spaceUnit)
    , m_timeUnit(timeUnit)
{
  std::replace(m_timeUnit.begin(), m_timeUnit.end(), '\\', '#');
  std::replace(m_spaceUnit.begin(), m_spaceUnit.end(), '\\', '#');
}

Mechanics::Device Mechanics::Device::fromFile(const std::string& path)
{
  return parseDevice(ConfigParser(path.c_str()));
}

void Mechanics::Device::addSensor(const Mechanics::Sensor& sensor)
{
  m_sensors.emplace_back(sensor);
  m_sensorMask.push_back(false);
}

void Mechanics::Device::addMaskedSensor() { m_sensorMask.push_back(true); }

void Mechanics::Device::applyAlignment(const Alignment& alignment)
{
  m_alignment = alignment;
  m_beamSlopeX = m_alignment.m_beamSlopeX;
  m_beamSlopeY = m_alignment.m_beamSlopeY;
  m_syncRatio = m_alignment.m_syncRatio;

  for (Index sensorId = 0; sensorId < getNumSensors(); ++sensorId) {
    Sensor* sensor = getSensor(sensorId);
    sensor->setLocalToGlobal(m_alignment.getLocalToGlobal(sensorId));
  }
  // TODO 2016-08-18 msmk: check number of sensors / id consistency
}

void Mechanics::Device::applyNoiseMask(const NoiseMask& noiseMask)
{
  m_noiseMask = noiseMask;

  for (Index sensorId = 0; sensorId < getNumSensors(); ++sensorId) {
    Sensor* sensor = getSensor(sensorId);
    sensor->setNoisyPixels(m_noiseMask.getMaskedPixels(sensorId));
  }
  // TODO 2016-08-18 msmk: check number of sensors / id consistency
}

//=========================================================
double Mechanics::Device::tsToTime(uint64_t timeStamp) const
{
  return (double)((timeStamp - getTimeStart()) / (double)getClockRate());
}

//=========================================================
unsigned int Mechanics::Device::getNumPixels() const
{
  unsigned int numPixels = 0;
  for (unsigned int nsens = 0; nsens < getNumSensors(); nsens++)
    numPixels += getSensor(nsens)->getNumPixels();
  return numPixels;
}

//=========================================================
void Mechanics::Device::print() const
{
  cout << "\nDEVICE:\n"
       << "  Name: '" << m_name << "'\n"
       << "  Clock rate: " << m_clockRate << "\n"
       << "  Read out window: " << m_readoutWindow << "\n"
       << "  Sensors: " << getNumSensors() << "\n"
       << "  Alignment: " << m_alignment.m_path << "\n"
       << "  Noisemask: " << m_noiseMask.getFileName() << endl;

  for (unsigned int nsens = 0; nsens < getNumSensors(); nsens++)
    getSensor(nsens)->print();
}
