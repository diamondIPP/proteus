#ifndef DEVICE_H
#define DEVICE_H

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "mechanics/alignment.h"
#include "mechanics/noisemask.h"
#include "mechanics/sensor.h"

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

  void addSensor(const Sensor& sensor);
  void addMaskedSensor();
  double tsToTime(uint64_t timeStamp) const;

  /** Store the alignment and apply it to all configured sensors. */
  void applyAlignment(const Alignment& alignment);
  /** Store the noise mask and apply to all configured sensors. */
  void applyNoiseMask(const NoiseMask& noiseMask);

  void setBeamSlopeX(double rotation) { m_beamSlopeX = rotation; }
  void setBeamSlopeY(double rotation) { m_beamSlopeY = rotation; }
  void setTimeStart(uint64_t timeStamp) { m_timeStart = timeStamp; }
  void setTimeEnd(uint64_t timeStamp) { m_timeEnd = timeStamp; }
  void setSyncRatio(double ratio) { m_syncRatio = ratio; }

  unsigned int getNumSensors() const { return m_sensors.size(); }
  Sensor* getSensor(unsigned int i) { return &m_sensors.at(i); }
  const Sensor* getSensor(unsigned int i) const { return &m_sensors.at(i); }

  unsigned int getNumPixels() const;

  const std::vector<bool>* getSensorMask() const { return &m_sensorMask; }

  const char* getName() const { return m_name.c_str(); }
  double getClockRate() const { return m_clockRate; }
  unsigned int getReadOutWindow() const { return m_readoutWindow; }
  const char* getSpaceUnit() const { return m_spaceUnit.c_str(); }
  const char* getTimeUnit() const { return m_timeUnit.c_str(); }
  double getBeamSlopeX() const { return m_beamSlopeX; }
  double getBeamSlopeY() const { return m_beamSlopeY; }
  uint64_t getTimeStart() const { return m_timeStart; }
  uint64_t getTimeEnd() const { return m_timeEnd; }
  double getSyncRatio() const { return m_syncRatio; }

  NoiseMask* getNoiseMask() { return &m_noiseMask; }
  Alignment* getAlignment() { return &m_alignment; }

  void print() const;

private:
  std::string m_name;
  std::vector<Sensor> m_sensors;
  Alignment m_alignment;
  NoiseMask m_noiseMask;
  double m_clockRate;
  unsigned int m_readoutWindow;
  uint64_t m_timeStart, m_timeEnd;
  double m_syncRatio;
  double m_beamSlopeX, m_beamSlopeY;
  std::string m_spaceUnit;
  std::string m_timeUnit;
  std::vector<bool> m_sensorMask;
};

} // namespace Mechanics

#endif // DEVICE_H
