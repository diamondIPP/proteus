#include "device.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <ostream>
#include <string>
#include <vector>

#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Device)

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

Mechanics::Device Mechanics::Device::fromFile(const std::string& path)
{
  using namespace Utils::Config;

  std::string dir = pathDirname(path);
  DEBUG("config base dir '", dir, "'");

  auto cfg = readConfig(path);
  Device device = fromConfig(cfg);

  auto cfgGeo = cfg.find("geometry");
  if (cfgGeo && cfgGeo->is<std::string>()) {
    auto p = pathRebaseIfRelative(cfgGeo->as<std::string>(), dir);
    device.setGeometry(Geometry::fromFile(p));
    device.m_pathGeometry = p;
  } else if (cfgGeo && cfgGeo->is<toml::Table>()) {
    device.setGeometry(Geometry::fromConfig(*cfgGeo));
  } else if (cfgGeo) {
    FAIL("invalid 'geometry' setting. must be string or object.");
  } else {
    FAIL("missing 'geometry' setting");
  }

  auto cfgMask = cfg.find("pixel_masks");
  // missing noise masks should not be treated as fatal errors
  // allow overlay of multiple noise masks
  if (cfgMask && cfgMask->is<std::vector<std::string>>()) {
    for (const auto& path : cfgMask->as<std::vector<std::string>>()) {
      try {
        auto fullPath = pathRebaseIfRelative(path, dir);
        device.applyPixelMasks(PixelMasks::fromFile(fullPath));
      } catch (const std::exception& e) {
        ERROR(e.what());
      }
    }
  } else if (cfgMask && cfgMask->is<toml::Table>()) {
    device.applyPixelMasks(PixelMasks::fromConfig(*cfgMask));
  } else if (cfgMask) {
    // the pixel_masks settings exists but does not have the right type. just
    // a missing pixel_masks settings is ok, but this must be fatal mistake.
    FAIL("invalid 'pixel_masks' setting. must be list of paths or object.");
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

  auto cfgTypes = cfg.get<toml::Table>("sensor_types");
  auto cfgSensors = cfg.get<toml::Array>("sensors");
  for (size_t isensor = 0; isensor < cfgSensors.size(); ++isensor) {
    toml::Value defaults = toml::Table{
        {"name", "sensor" + std::to_string(isensor)}, {"is_masked", false}};
    toml::Value cfgSensor =
        Utils::Config::withDefaults(cfgSensors[isensor], defaults);
    if (cfgSensor.get<bool>("is_masked")) {
      device.addMaskedSensor();
      continue;
    }

    auto name = cfgSensor.get<std::string>("name");
    auto typeName = cfgSensor.get<std::string>("type");
    if (cfgTypes.count(typeName) == 0) {
      throw std::runtime_error("Device: sensor type '" + typeName +
                               "' does not exist");
    }
    auto cfgType = cfgTypes[typeName];
    auto cfgRegions = cfgType.find("regions");
    auto measurement =
        Sensor::measurementFromName(cfgType.get<std::string>("measurement"));

    // construct sensor regions if they are defined
    std::vector<Sensor::Region> regions;
    if (cfgRegions && cfgRegions->is<toml::Array>()) {
      for (size_t iregion = 0; iregion < cfgRegions->size(); ++iregion) {
        toml::Value defaults =
            toml::Table{{"name", "region" + std::to_string(iregion)},
                        {"col_min", INT_MIN},
                        // upper limit in the configuration is inclusive, but in
                        // the code the interval is half-open w/ the upper limit
                        // being exclusive. Ensure +1 is still within the
                        // numerical limits.
                        {"col_max", INT_MAX - 1},
                        {"row_min", INT_MIN},
                        {"row_max", INT_MAX - 1}};
        toml::Value cfgRegion =
            Utils::Config::withDefaults(*cfgRegions->find(iregion), defaults);

        Sensor::Region region;
        region.name = cfgRegion.get<std::string>("name");
        region.areaPixel = Sensor::Area(
            Sensor::Area::AxisInterval(cfgRegion.get<int>("col_min"),
                                       cfgRegion.get<int>("col_max") + 1),
            Sensor::Area::AxisInterval(cfgRegion.get<int>("row_min"),
                                       cfgRegion.get<int>("row_max") + 1));
        regions.emplace_back(std::move(region));
      }
    }

    device.addSensor(Sensor(
        isensor, name, measurement, cfgType.get<int>("cols"),
        cfgType.get<int>("rows"), cfgType.get<double>("pitch_col"),
        cfgType.get<double>("pitch_row"), cfgType.get<double>("thickness"),
        cfgType.get<double>("x_x0"), regions));
  }
  return device;
}

void Mechanics::Device::addSensor(const Sensor& sensor)
{
  // TODO 2017-02-07 msmk: assumes ids are indices from 0 to n_sensors w/o gaps
  m_sensorIds.emplace_back(sensor.id());
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

void Mechanics::Device::applyPixelMasks(const PixelMasks& pixelMasks)
{
  m_pixelMasks = pixelMasks;

  for (Index sensorId = 0; sensorId < numSensors(); ++sensorId) {
    Sensor* sensor = getSensor(sensorId);
    sensor->setMaskedPixels(m_pixelMasks.getMaskedPixels(sensorId));
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
  m_pixelMasks.print(os, prefix + "  ");

  os.flush();
}

std::vector<Index> Mechanics::sortedByZ(const Device& device,
                                        const std::vector<Index>& sensorIds)
{
  std::vector<Index> sorted(std::begin(sensorIds), std::end(sensorIds));
  std::sort(sorted.begin(), sorted.end(), CompareSensorIdZ{device});
  return sorted;
}
