#ifndef PT_DEVICE_H
#define PT_DEVICE_H

#include <cmath>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "mechanics/alignment.h"
#include "mechanics/noisemask.h"
#include "mechanics/sensor.h"
#include "utils/config.h"
#include "utils/definitions.h"

namespace Mechanics {

class Device {
public:
  Device(const std::string& name,
         double clockRate,
         unsigned int readoutWindow,
         const std::string& spaceUnit = std::string(),
         const std::string& timeUnit = std::string());

  /** Construct device from a configuration file. */
  static Device fromFile(const std::string& path);
  /** Construct device from a configuration object. */
  static Device fromConfig(const toml::Value& cfg);

  const std::string& name() const { return m_name; }
  const std::string& pathAlignment() const { return m_pathAlignment; }
  const std::string& pathNoiseMask() const { return m_pathNoiseMask; }

  void addSensor(const Sensor& sensor);
  void addMaskedSensor();
  Index numSensors() const { return static_cast<Index>(m_sensors.size()); }
  Sensor* getSensor(Index i) { return &m_sensors.at(i); }
  const Sensor* getSensor(Index i) const { return &m_sensors.at(i); }

  /** Store the geometry and apply it to all configured sensors. */
  void setGeometry(const Alignment& alignment);
  const Alignment& geometry() const { return m_geometry; }

  /** Store the noise mask and apply to all configured sensors. */
  void applyNoiseMask(const NoiseMask& noiseMask);
  const NoiseMask& noiseMask() const { return m_noiseMask; }

  uint64_t timeStampStart() const { return m_timeStart; }
  uint64_t timeStampEnd() const { return m_timeEnd; }
  double clockRate() const { return m_clockRate; }
  double tsToTime(uint64_t timeStamp) const;
  void setTimeStampRange(uint64_t start, uint64_t end);

  unsigned int getNumPixels() const;
  const char* getName() const { return m_name.c_str(); }
  double getClockRate() const { return m_clockRate; }
  unsigned int getReadOutWindow() const { return m_readoutWindow; }
  const char* getSpaceUnit() const { return m_spaceUnit.c_str(); }
  const char* getTimeUnit() const { return m_timeUnit.c_str(); }
  double getBeamSlopeX() const;
  double getBeamSlopeY() const;
  uint64_t getTimeStart() const { return m_timeStart; }
  uint64_t getTimeEnd() const { return m_timeEnd; }
  double getSyncRatio() const { return m_geometry.syncRatio(); }
  /** \deprecated Use alignment directly */
  void setSyncRatio(double ratio) { m_geometry.setSyncRatio(ratio); }
  unsigned int getNumSensors() const { return m_sensors.size(); }

  const std::vector<bool>* getSensorMask() const { return &m_sensorMask; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  std::string m_name;
  std::vector<Sensor> m_sensors;
  Alignment m_geometry;
  NoiseMask m_noiseMask;
  double m_clockRate;
  unsigned int m_readoutWindow;
  uint64_t m_timeStart, m_timeEnd;
  std::string m_pathAlignment, m_pathNoiseMask;
  std::string m_spaceUnit;
  std::string m_timeUnit;
  std::vector<bool> m_sensorMask;
};

} // namespace Mechanics

#endif // PT_DEVICE_H
