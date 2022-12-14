// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#include "device.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <ostream>
#include <string>
#include <vector>

#include "utils/config.h"
#include "utils/logger.h"

namespace proteus {

Device Device::fromFile(const std::string& path,
                        const std::string& pathGeometry)
{
  auto dir = pathDirname(path);
  DEBUG("config base dir '", dir, "'");

  auto cfg = configRead(path);
  INFO("read device from '", path, "'");

  Device dev = fromConfig(cfg);

  // load all pixel masks
  auto cfgMask = cfg.find("pixel_masks");
  if (cfgMask && cfgMask->is<std::vector<std::string>>()) {
    for (const auto& path : cfgMask->as<std::vector<std::string>>()) {
      auto fullPath = pathRebaseIfRelative(path, dir);
      dev.applyPixelMasks(PixelMasks::fromFile(fullPath));
    }
  } else if (cfgMask && cfgMask->is<toml::Table>()) {
    dev.applyPixelMasks(PixelMasks::fromConfig(*cfgMask));
  } else if (cfgMask) {
    // the pixel_masks settings exists but does not have the right type. just
    // a missing pixel_masks settings is ok, but this must be fatal mistake.
    FAIL("invalid 'pixel_masks' setting. must be array of strings or object.");
  }

  // load geometry
  if (!pathGeometry.empty()) {
    dev.setGeometry(Geometry::fromFile(pathGeometry));
  } else {
    auto cfgGeo = cfg.find("geometry");
    if (cfgGeo && cfgGeo->is<std::string>()) {
      auto p = pathRebaseIfRelative(cfgGeo->as<std::string>(), dir);
      dev.setGeometry(Geometry::fromFile(p));
    } else if (cfgGeo && cfgGeo->is<toml::Table>()) {
      dev.setGeometry(Geometry::fromConfig(*cfgGeo));
    } else if (cfgGeo) {
      FAIL("invalid 'geometry' setting. must be string or object.");
    } else {
      FAIL("missing 'geometry' setting");
    }
  }

  return dev;
}

Device Device::fromConfig(const toml::Value& cfg)
{
  // deprecation checks
  if (cfg.has("device")) {
    WARN("The '[device]' configuration section is deprecated and will not be "
         "used");
  }
  for (const auto& it : cfg.get<toml::Table>("sensor_types")) {
    if (it.second.has("thickness")) {
      WARN("The 'thickness' setting for sensor type '" + it.first +
           "' is deprecated and will not be used");
    }
  }

  // WARNING
  // upper limits in the configuration file are inclusive, but in
  // the code the interval is always half-open w/ the upper limit
  // being exclusive.

  // defaults for optional sensor type entries
  // the values are taken for FEI4 sensors; these values were hardcorded
  // before and are now used as defaults to keep backward compatibility
  auto defaultsType = toml::Table{
      {"timestamp_min", 0},
      {"timestamp_max", 15},
      {"value_max", 15},
      {"pitch_timestamp", 1.0},
  };
  auto defaultsRegion = toml::Table{
      {"col_min", INT_MIN},
      {"col_max", INT_MAX - 1},
      {"row_min", INT_MIN},
      {"row_max", INT_MAX - 1},
  };
  // fill defaults for optional sensor type settings
  auto configTypes = toml::Table{};
  for (const auto& it : cfg.get<toml::Table>("sensor_types")) {
    auto configType = configWithDefaults(it.second, defaultsType);
    auto configRegions = toml::Array{};
    if (configType.has("regions")) {
      for (const auto& configRegion : configType.get<toml::Array>("regions")) {
        configRegions.push_back(
            configWithDefaults(configRegion, defaultsRegion));
      }
    }
    configType["regions"] = std::move(configRegions);
    configTypes[it.first] = std::move(configType);
  }

  // construct device and sensors
  Device device;
  Index isensor = 0;
  for (const auto& configSensor : cfg.get<toml::Array>("sensors")) {

    // sensor-specific settings
    auto id = isensor++;
    auto name =
        (configSensor.has("name") ? configSensor.get<std::string>("name")
                                  : "sensor" + std::to_string(id));
    auto type = configSensor.get<std::string>("type");

    // get sensor type configuration
    if (configTypes.count(type) == 0) {
      THROW("sensor type '", type, "' is undefined");
    }
    const auto& config = configTypes.at(type);

    // construct basic sensor
    auto sensor = Sensor(
        id, name,
        Sensor::measurementFromName(config.get<std::string>("measurement")),
        static_cast<Index>(config.get<int>("cols")),
        static_cast<Index>(config.get<int>("rows")),
        // see comments above for +1
        config.get<int>("timestamp_min"), config.get<int>("timestamp_max") + 1,
        config.get<int>("value_max") + 1, config.get<double>("pitch_col"),
        config.get<double>("pitch_row"), config.get<double>("pitch_timestamp"),
        config.get<double>("x_x0"));

    // add regions if defined
    for (const auto& r : config.get<toml::Array>("regions")) {
      sensor.addRegion(r.get<std::string>("name"), r.get<int>("col_min"),
                       r.get<int>("col_max") + 1, r.get<int>("row_min"),
                       r.get<int>("row_max") + 1);
    }

    device.addSensor(std::move(sensor));
  }
  return device;
}

Sensor::Volume Device::boundingBox() const
{
  auto box = Sensor::Volume::Empty();
  for (const auto& sensor : m_sensors) {
    box.enclose(sensor.projectedBoundingBox());
  }
  return box;
}

Vector4 Device::minimumPitch() const
{
  Vector4 pitch = Vector4::Constant(std::numeric_limits<Scalar>::max());
  for (const auto& sensor : m_sensors) {
    pitch = pitch.cwiseMin(sensor.projectedPitch());
  }
  return pitch;
}

void Device::addSensor(Sensor&& sensor)
{
  // TODO 2017-02-07 msmk: assumes ids are indices from 0 to n_sensors w/o gaps
  m_sensors.emplace_back(std::move(sensor));
  m_sensorIds.emplace_back(m_sensors.back().id());
}

void Device::setGeometry(const Geometry& geometry)
{
  m_geometry = geometry;
  // update geometry-dependent sensor properties
  for (auto& sensor : m_sensors) {
    sensor.updateGeometry(m_geometry);
  }
  // TODO 2016-08-18 msmk: check number of sensors / id consistency
}

void Device::applyPixelMasks(const PixelMasks& pixelMasks)
{
  m_pixelMasks.merge(pixelMasks);

  for (auto id : m_sensorIds) {
    getSensor(id).m_pixelMask = DenseMask(m_pixelMasks.getMaskedPixels(id));
  }
  // TODO 2016-08-18 msmk: check number of sensors / id consistency
}

void Device::print(std::ostream& os, const std::string& prefix) const
{
  for (auto sensorId : m_sensorIds) {
    os << prefix << "sensor " << sensorId << ":\n";
    getSensor(sensorId).print(os, prefix + "  ");
  }
  os << prefix << "geometry:\n";
  m_geometry.print(os, prefix + "  ");
  os << prefix << "noise mask:\n";
  m_pixelMasks.print(os, prefix + "  ");
  os.flush();
}

} // namespace proteus
