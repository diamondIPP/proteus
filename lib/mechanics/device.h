#ifndef PT_DEVICE_H
#define PT_DEVICE_H

#include <cmath>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "mechanics/geometry.h"
#include "mechanics/pixelmasks.h"
#include "mechanics/sensor.h"
#include "utils/config.h"
#include "utils/definitions.h"

namespace Mechanics {

class Device {
public:
  /** Construct device from a configuration file.
   *
   * \param path         Path to the device file
   * \param pathGeometry Path to a geometry file
   *
   * If the optional geometry path is non-empty, the geometry config is read
   * from there and any information in the device file is ignored.
   */
  static Device fromFile(const std::string& path,
                         const std::string& pathGeometry = std::string());
  /** Construct device from a configuration object. */
  static Device fromConfig(const toml::Value& cfg);

  const std::vector<Index>& sensorIds() const { return m_sensorIds; }
  Index numSensors() const { return static_cast<Index>(m_sensors.size()); }
  Sensor& getSensor(Index i) { return m_sensors.at(i); }
  const Sensor& getSensor(Index i) const { return m_sensors.at(i); }

  /** Store the geometry and apply it to all configured sensors. */
  void setGeometry(const Geometry& geometry);
  const Geometry& geometry() const { return m_geometry; }

  /** Store the pixel masks and apply to all configured sensors. */
  void applyPixelMasks(const PixelMasks& masks);
  const PixelMasks& pixelMasks() const { return m_pixelMasks; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  Device() = default;
  void addSensor(Sensor&& sensor);

  std::vector<Index> m_sensorIds;
  std::vector<Sensor> m_sensors;
  Geometry m_geometry;
  PixelMasks m_pixelMasks;
};

} // namespace Mechanics

#endif // PT_DEVICE_H
