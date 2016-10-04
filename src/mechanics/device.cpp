#include "device.h"

#include <algorithm>
#include <cassert>
#include <ostream>
#include <string>
#include <vector>

#include "utils/configparser.h"
#include "utils/logger.h"

using Utils::logger;

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
    , m_spaceUnit(spaceUnit)
    , m_timeUnit(timeUnit)
{
  std::replace(m_timeUnit.begin(), m_timeUnit.end(), '\\', '#');
  std::replace(m_spaceUnit.begin(), m_spaceUnit.end(), '\\', '#');
}

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

        Mechanics::Sensor sensor(
            name, cols, rows, pitchX, pitchY, digi, depth, xox0);
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
    else
      throw std::runtime_error("Device: failed to parse row, key='" + row->key +
                               '\'');
  }
}

static Mechanics::Device parseDevice(const std::string& path)
{
  ConfigParser config(path.c_str());
  std::string name;
  std::string pathAlignment;
  std::string pathNoiseMask;
  std::string spaceUnit;
  std::string timeUnit;
  double clockRate = 1;
  double readOutWindow = 0;

  for (unsigned int i = 0; i < config.getNumRows(); i++) {
    const ConfigParser::Row* row = config.getRow(i);

    if (row->isHeader && !row->header.compare("End Device")) {

      INFO("device config: '", path, "'\n");
      INFO("alignment config: '", pathAlignment, "'\n");
      INFO("noise mask config: '", pathNoiseMask, "'\n");

      Mechanics::Device device(
          name, clockRate, readOutWindow, spaceUnit, timeUnit);
      Mechanics::Alignment alignment;

      parseSensors(config, device, alignment);

      if (!pathAlignment.empty()) {
        device.applyAlignment(Mechanics::Alignment::fromFile(pathAlignment));
        // device.m_pathAlignment = pathAlignment;
      } else {
        device.applyAlignment(alignment);
      }
      if (!pathNoiseMask.empty()) {
        device.applyNoiseMask(Mechanics::NoiseMask::fromFile(pathNoiseMask));
        // device.m_pathNoiseMask = pathNoiseMask;
      }
      return device;
    }

    if (row->isHeader)
      continue;

    if (row->header.compare("Device"))
      continue; // Skip non-device rows

    if (!row->key.compare("name"))
      name = row->value;
    else if (!row->key.compare("alignment"))
      pathAlignment = row->value;
    else if (!row->key.compare("noise mask"))
      pathNoiseMask = row->value;
    else if (!row->key.compare("clock"))
      clockRate = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("window"))
      readOutWindow = ConfigParser::valueToNumerical(row->value);
    else if (!row->key.compare("space unit"))
      spaceUnit = row->value;
    else if (!row->key.compare("time unit"))
      timeUnit = row->value;
    else
      throw std::runtime_error("Device: Failed to parse row, key='" + row->key +
                               '\'');
  }

  // Control shouldn't arrive at this point
  throw std::runtime_error("No device was parsed.");
}

Mechanics::Device Mechanics::Device::fromFile(const std::string& path)
{
    using Utils::Config::readConfig;

    std::string ext(path.substr(path.find_last_of('.') + 1));

    if (ext == "toml") {
        auto cfg = readConfig(path);
        auto cfgAlign = cfg.find("alignment");
        auto cfgNoise = cfg.find("noise_mask");
        if (cfgAlign && cfgAlign->is<std::string>())
          cfg["alignment"] = readConfig(cfgAlign->as<std::string>());
        if (cfgNoise && cfgNoise->is<std::string>())
          cfg["noise_mask"] = readConfig(cfgNoise->as<std::string>());
        return fromConfig(cfg);
    }
    return parseDevice(path);
}

Mechanics::Device Mechanics::Device::fromConfig(const toml::Value& cfg)
{
  Device device(cfg.get<std::string>("device.name"),
                cfg.get<double>("device.clock"),
                cfg.get<int>("device.window"),
                cfg.get<std::string>("device.space_unit"),
                cfg.get<std::string>("device.time_unit"));

  auto types = cfg.get<toml::Table>("sensor_types");
  auto sensors = cfg.get<toml::Array>("sensors");

  for (size_t i = 0; i < sensors.size(); ++i) {
    auto sensor = sensors[i];
    auto name = Utils::Config::get(sensor, "name", "plane" + std::to_string(i));
    auto typeName = sensor.get<std::string>("type");
    auto type = types[typeName];

    device.addSensor(Sensor(name,
                            type.get<int>("cols"),
                            type.get<int>("rows"),
                            type.get<double>("pitch_col"),
                            type.get<double>("pitch_row"),
                            type.get<bool>("is_digital"),
                            type.get<double>("depth"),
                            type.get<double>("x_x0")));
  }

  auto cfgAlign = cfg.find("alignment");
  auto cfgNoise = cfg.find("noise_mask");
  if (cfgAlign)
    device.applyAlignment(Alignment::fromConfig(*cfgAlign));
  if (cfgNoise)
    device.applyNoiseMask(NoiseMask::fromConfig(*cfgNoise));

  return device;
}

void Mechanics::Device::addSensor(const Sensor& sensor)
{
  m_sensors.emplace_back(sensor);
  m_sensorMask.push_back(false);
}

void Mechanics::Device::addMaskedSensor() { m_sensorMask.push_back(true); }

void Mechanics::Device::applyAlignment(const Alignment& alignment)
{
  m_alignment = alignment;

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

double Mechanics::Device::tsToTime(uint64_t timeStamp) const
{
  return (double)((timeStamp - getTimeStart()) / (double)getClockRate());
}

void Mechanics::Device::setTimeStampRange(uint64_t start, uint64_t end)
{
  if (end < start)
    throw std::runtime_error("end time stamp must come after start");
  m_timeStart = start;
  m_timeEnd = end;
}

unsigned int Mechanics::Device::getNumPixels() const
{
  unsigned int numPixels = 0;
  for (unsigned int nsens = 0; nsens < getNumSensors(); nsens++)
    numPixels += getSensor(nsens)->getNumPixels();
  return numPixels;
}

double Mechanics::Device::getBeamSlopeX() const
{
  auto dir = m_alignment.beamDirection();
  return dir.x() / dir.z();
}

double Mechanics::Device::getBeamSlopeY() const
{
  auto dir = m_alignment.beamDirection();
  return dir.y() / dir.z();
}

void Mechanics::Device::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "name: " << m_name << '\n';
  os << prefix << "clock rate: " << m_clockRate << '\n';
  os << prefix << "readout window: " << m_readoutWindow << '\n';
  for (Index sensorId = 0; sensorId < getNumSensors(); ++sensorId) {
    os << prefix << "sensor " << sensorId << ":\n";
    getSensor(sensorId)->print(os, prefix + "  ");
  }

  os << prefix << "alignment:\n";
  if (!m_pathAlignment.empty())
    os << prefix << "  path: " << m_pathAlignment << '\n';
  m_alignment.print(os, prefix + "  ");

  os << prefix << "noise mask:\n";
  if (!m_pathNoiseMask.empty())
    os << prefix << "  path: " << m_pathNoiseMask << '\n';
  m_noiseMask.print(os, prefix + "  ");

  os.flush();
}
