#include "device.h"

#include <algorithm>
#include <cassert>
#include <ostream>
#include <string>
#include <vector>

#include "utils/configparser.h"
#include "utils/logger.h"

PT_SETUP_GLOBAL_LOGGER

Mechanics::Device::Device(const std::string& name,
                          double clockRate,
                          unsigned int readoutWindow,
                          const std::string& spaceUnit,
                          const std::string& timeUnit)
    : m_name(name)
    , m_clockRate(clockRate)
    , m_readoutWindow(readoutWindow)
    , m_timestamp0(0)
    , m_timestamp1(0)
    , m_spaceUnit(spaceUnit)
    , m_timeUnit(timeUnit)
{
  std::replace(m_timeUnit.begin(), m_timeUnit.end(), '\\', '#');
  std::replace(m_spaceUnit.begin(), m_spaceUnit.end(), '\\', '#');
}

static void parseSensors(const ConfigParser& config,
                         Mechanics::Device& device,
                         Mechanics::Geometry& alignment)
{
  using Mechanics::Sensor;

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

        Sensor::Measurement m = (digi ? Sensor::Measurement::PixelBinary
                                      : Sensor::Measurement::PixelTot);
        Sensor sensor(sensorCounter, name, m, cols, rows, pitchX, pitchY, depth,
                      xox0);
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

      INFO("device config: '", path, "'");
      INFO("alignment config: '", pathAlignment, "'");
      INFO("noise mask config: '", pathNoiseMask, "'");

      Mechanics::Device device(name, clockRate, readOutWindow, spaceUnit,
                               timeUnit);
      Mechanics::Geometry alignment;

      parseSensors(config, device, alignment);

      if (!pathAlignment.empty()) {
        device.setGeometry(Mechanics::Geometry::fromFile(pathAlignment));
      } else {
        device.setGeometry(alignment);
      }
      if (!pathNoiseMask.empty()) {
        device.applyNoiseMask(Mechanics::NoiseMask::fromFile(pathNoiseMask));
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
  using namespace Utils::Config;

  Device device;
  std::string dir = pathDirname(path);
  DEBUG("config base dir '", dir, "'");

  if (pathExtension(path) == "toml") {
    auto cfg = readConfig(path);
    device = fromConfig(cfg);

    auto cfgGeo = cfg.find("geometry");
    if (cfgGeo && cfgGeo->is<std::string>()) {
      auto p = pathRebaseIfRelative(cfgGeo->as<std::string>(), dir);
      device.setGeometry(Geometry::fromFile(p));
      device.m_pathGeometry = p;
    } else if (cfgGeo) {
      device.setGeometry(Geometry::fromConfig(*cfgGeo));
    }

    auto cfgMask = cfg.find("noise_mask");
    // missing noise masks should not be treated as fatal errors
    // allow overlay of multiple noise masks
    if (cfgMask && cfgMask->is<std::vector<std::string>>()) {
      NoiseMask combined;
      auto paths = cfgMask->as<std::vector<std::string>>();
      for (auto it = paths.begin(); it != paths.end(); ++it) {
        try {
          auto path = pathRebaseIfRelative(*it, dir);
          auto mask = NoiseMask::fromFile(path);
          combined.merge(mask);
        } catch (const std::exception& e) {
          ERROR(e.what());
        }
      }
      device.applyNoiseMask(combined);
    } else if (cfgMask && cfgMask->is<std::string>()) {
      auto path = pathRebaseIfRelative(cfgMask->as<std::string>(), dir);
      try {
        device.applyNoiseMask(NoiseMask::fromFile(path));
        device.m_pathNoiseMask = path;
      } catch (const std::exception& e) {
        ERROR(e.what());
      }
    } else if (cfgMask) {
      device.applyNoiseMask(NoiseMask::fromConfig(*cfgMask));
    }
  } else {
    // fall-back to old format
    device = parseDevice(path);
  }
  INFO("read device from '", path, "'");
  return device;
}

Mechanics::Device Mechanics::Device::fromConfig(const toml::Value& cfg)
{
  Device device(cfg.get<std::string>("device.name"),
                cfg.get<double>("device.clock"), cfg.get<int>("device.window"),
                cfg.get<std::string>("device.space_unit"),
                cfg.get<std::string>("device.time_unit"));

  auto types = cfg.get<toml::Table>("sensor_types");
  auto sensors = cfg.get<toml::Array>("sensors");
  for (size_t i = 0; i < sensors.size(); ++i) {
    toml::Value defaults = toml::Table{{"name", "plane" + std::to_string(i)},
                                       {"is_masked", false}};
    toml::Value sensor = Utils::Config::withDefaults(sensors[i], defaults);
    if (sensor.get<bool>("is_masked")) {
      device.addMaskedSensor();
      continue;
    }
    auto name = sensor.get<std::string>("name");
    auto typeName = sensor.get<std::string>("type");
    if (types.count(typeName) == 0) {
      throw std::runtime_error("Device: sensor type '" + typeName +
                               "' does not exist");
    }
    auto type = types[typeName];
    auto measurement =
        Sensor::measurementFromName(type.get<std::string>("measurement"));
    device.addSensor(Sensor(
        i, name, measurement, type.get<int>("cols"), type.get<int>("rows"),
        type.get<double>("pitch_col"), type.get<double>("pitch_row"),
        type.get<double>("thickness"), type.get<double>("x_x0")));
  }
  return device;
}

void Mechanics::Device::addSensor(const Sensor& sensor)
{
  m_sensors.emplace_back(sensor);
  m_sensorMask.push_back(false);
}

void Mechanics::Device::addMaskedSensor() { m_sensorMask.push_back(true); }

void Mechanics::Device::setGeometry(const Geometry& geometry)
{
  m_geometry = geometry;

  for (Index sensorId = 0; sensorId < numSensors(); ++sensorId) {
    Sensor* sensor = getSensor(sensorId);
    sensor->setLocalToGlobal(m_geometry.getLocalToGlobal(sensorId));
  }
  // TODO 2016-08-18 msmk: check number of sensors / id consistency
}

void Mechanics::Device::applyNoiseMask(const NoiseMask& noiseMask)
{
  m_noiseMask = noiseMask;

  for (Index sensorId = 0; sensorId < numSensors(); ++sensorId) {
    Sensor* sensor = getSensor(sensorId);
    sensor->setNoisyPixels(m_noiseMask.getMaskedPixels(sensorId));
  }
  // TODO 2016-08-18 msmk: check number of sensors / id consistency
}

double Mechanics::Device::tsToTime(uint64_t timestamp) const
{
  return (double)((timestamp - timestampStart()) / (double)clockRate());
}

void Mechanics::Device::setTimestampRange(uint64_t ts0, uint64_t ts1)
{
  if (ts1 < ts0)
    throw std::runtime_error("start timestamp must come before end");
  m_timestamp0 = ts0;
  m_timestamp1 = ts1;
}

double Mechanics::Device::getBeamSlopeX() const
{
  auto dir = m_geometry.beamDirection();
  return dir.x() / dir.z();
}

double Mechanics::Device::getBeamSlopeY() const
{
  auto dir = m_geometry.beamDirection();
  return dir.y() / dir.z();
}

void Mechanics::Device::print(std::ostream& os, const std::string& prefix) const
{
  os << prefix << "name: " << m_name << '\n';
  os << prefix << "clock rate: " << m_clockRate << '\n';
  os << prefix << "readout window: " << m_readoutWindow << '\n';
  for (Index sensorId = 0; sensorId < numSensors(); ++sensorId) {
    os << prefix << "sensor " << sensorId << ":\n";
    getSensor(sensorId)->print(os, prefix + "  ");
  }
  os << prefix << "geometry:\n";
  if (!m_pathGeometry.empty())
    os << prefix << "  path: " << m_pathGeometry << '\n';
  m_geometry.print(os, prefix + "  ");
  os << prefix << "noise mask:\n";
  if (!m_pathNoiseMask.empty())
    os << prefix << "  path: " << m_pathNoiseMask << '\n';
  m_noiseMask.print(os, prefix + "  ");

  os.flush();
}
