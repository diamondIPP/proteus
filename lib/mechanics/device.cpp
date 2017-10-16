#include "device.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <ostream>
#include <string>
#include <vector>

#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Device)

Mechanics::Device Mechanics::Device::fromFile(const std::string& path,
                                              const std::string& pathGeometry)
{
  using namespace Utils::Config;

  auto dir = pathDirname(path);
  DEBUG("config base dir '", dir, "'");

  auto cfg = readConfig(path);
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

Mechanics::Device Mechanics::Device::fromConfig(const toml::Value& cfg)
{
  Device device;

  if (cfg.has("device"))
    ERROR("device configuration is deprecated and will not be used");

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

  // update geometry-dependent sensor properties
  auto setProjectedEnvelope = [](const Plane& plane, Sensor& sensor) {
    using Area = Sensor::Area;

    Area pix = sensor.sensitiveAreaLocal();
    // TODO 2016-08 msmk: find a smarter way to this, but its Friday
    // transform each corner of the sensitive rectangle
    Vector3 minMin = plane.toGlobal(Vector2(pix.min(0), pix.min(1)));
    Vector3 minMax = plane.toGlobal(Vector2(pix.min(0), pix.max(1)));
    Vector3 maxMin = plane.toGlobal(Vector2(pix.max(0), pix.min(1)));
    Vector3 maxMax = plane.toGlobal(Vector2(pix.max(0), pix.max(1)));

    std::array<double, 4> xs = {{minMin[0], minMax[0], maxMin[0], maxMax[0]}};
    std::array<double, 4> ys = {{minMin[1], minMax[1], maxMin[1], maxMax[1]}};

    sensor.m_projEnvelopeXY =
        Area(Area::AxisInterval(*std::min_element(xs.begin(), xs.end()),
                                *std::max_element(xs.begin(), xs.end())),
             Area::AxisInterval(*std::min_element(ys.begin(), ys.end()),
                                *std::max_element(ys.begin(), ys.end())));
  };
  auto setProjectedPitch = [](const Plane& plane, Sensor& sensor) {
    Vector3 pitchUVW(sensor.pitchCol(), sensor.pitchRow(), 0);
    Vector3 pitchXYZ = plane.rotation * pitchUVW;
    // only interested in absolute values not in direction
    for (int i : {0, 1, 2})
      pitchXYZ[i] = std::abs(pitchXYZ[i]);
    sensor.m_projPitchXY = pitchXYZ.Sub<Vector2>(0);
  };

  for (Index sensorId = 0; sensorId < numSensors(); ++sensorId) {
    setProjectedEnvelope(m_geometry.getPlane(sensorId), *getSensor(sensorId));
    setProjectedPitch(m_geometry.getPlane(sensorId), *getSensor(sensorId));
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

void Mechanics::Device::print(std::ostream& os, const std::string& prefix) const
{
  for (Index sensorId = 0; sensorId < numSensors(); ++sensorId) {
    os << prefix << "sensor " << sensorId << ":\n";
    getSensor(sensorId)->print(os, prefix + "  ");
  }
  os << prefix << "geometry:\n";
  m_geometry.print(os, prefix + "  ");
  os << prefix << "noise mask:\n";
  m_pixelMasks.print(os, prefix + "  ");
  os.flush();
}
