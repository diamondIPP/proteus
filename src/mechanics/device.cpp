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

        Sensor::Measurement m = (digi ? Sensor::Measurement::PIXEL_BINARY
                                      : Sensor::Measurement::PIXEL_TOT);
        Sensor sensor(name, m, cols, rows, pitchX, pitchY, depth, xox0);
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

      Mechanics::Device device(name, clockRate, readOutWindow, spaceUnit,
                               timeUnit);
      Mechanics::Alignment alignment;

      parseSensors(config, device, alignment);

      if (!pathAlignment.empty()) {
        device.applyAlignment(Mechanics::Alignment::fromFile(pathAlignment));
      } else {
        device.applyAlignment(alignment);
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

  std::string dir = pathDirname(path);
  DEBUG("config base dir '", dir, "'\n");

  if (pathExtension(path) == "toml") {
    auto cfg = readConfig(path);
    auto device = fromConfig(readConfig(path));

    auto cfgAlign = cfg.find("alignment");
    if (cfgAlign && cfgAlign->is<std::string>()) {
      auto p = pathRebaseIfRelative(cfgAlign->as<std::string>(), dir);
      device.applyAlignment(Alignment::fromFile(p));
      device.m_pathAlignment = p;
    } else if (cfgAlign) {
      device.applyAlignment(Alignment::fromConfig(*cfgAlign));
    }

    auto cfgMask = cfg.find("noise_mask");
    // allow overlay of multiple noise masks
    if (cfgMask && cfgMask->is<std::vector<std::string>>()) {
      NoiseMask combined;
      auto paths = cfgMask->as<std::vector<std::string>>();
      for (auto it = paths.begin(); it != paths.end(); ++it) {
        auto p = pathRebaseIfRelative(*it, dir);
        combined.merge(NoiseMask::fromFile(p));
      }
      device.applyNoiseMask(combined);
    } else if (cfgMask && cfgMask->is<std::string>()) {
      auto p = pathRebaseIfRelative(cfgMask->as<std::string>(), dir);
      device.applyNoiseMask(NoiseMask::fromFile(p));
      device.m_pathNoiseMask = p;
    } else if (cfgMask) {
      device.applyNoiseMask(NoiseMask::fromConfig(*cfgMask));
    }

    return device;
  }
  // fall-back to old format
  return parseDevice(path);
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
    device.addSensor(
        Sensor(name, measurement, type.get<int>("cols"), type.get<int>("rows"),
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

void Mechanics::Device::applyAlignment(const Alignment& alignment)
{
  m_alignment = alignment;

  for (Index sensorId = 0; sensorId < numSensors(); ++sensorId) {
    Sensor* sensor = getSensor(sensorId);
    sensor->setLocalToGlobal(m_alignment.getLocalToGlobal(sensorId));
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

double Mechanics::Device::tsToTime(uint64_t timeStamp) const
{
  return (double)((timeStamp - timeStampStart()) / (double)clockRate());
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
  for (unsigned int nsens = 0; nsens < numSensors(); nsens++)
    numPixels += getSensor(nsens)->numPixels();
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
  for (Index sensorId = 0; sensorId < numSensors(); ++sensorId) {
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
