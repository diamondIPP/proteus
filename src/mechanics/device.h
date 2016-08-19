#ifndef DEVICE_H
#define DEVICE_H

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "alignment.h"
#include "noisemask.h"

namespace Mechanics {

class Sensor;

class Device {
public:
  Device(const std::string& name,
         double clockRate,
         unsigned int readoutWindow,
         const std::string& spaceUnit = std::string(),
         const std::string& timeUnit = std::string());
  Device(const char* name,
         const char* alignmentName = "",
         const char* noiseMaskName = "",
         double clockRate = 0,
         unsigned int readOutWindow = 0,
         const char* spaceUnit = "",
         const char* timeUnit = "");
  ~Device();
  /** Construct device from a configuration file. */
  static Device fromFile(const std::string& path);

  void addSensor(Sensor* sensor);
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
  Sensor* getSensor(unsigned int n) const;

  unsigned int getNumPixels() const;

  const std::vector<bool>* getSensorMask() const { return &m_sensorMask; }

  const char* getName() const { return m_name.c_str(); }
  double getClockRate() const { return m_clockRate; }
  unsigned int getReadOutWindow() const { return m_readOutWindow; }
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
  double m_clockRate;
  unsigned int m_readOutWindow;
  std::string m_spaceUnit;
  std::string m_timeUnit;
  double m_beamSlopeX;
  double m_beamSlopeY;
  uint64_t m_timeStart;
  uint64_t m_timeEnd;
  double m_syncRatio;

  std::vector<Sensor*> m_sensors;
  std::vector<bool> m_sensorMask;

  Alignment m_alignment;
  NoiseMask m_noiseMask;
};

} // namespace Mechanics

#endif // DEVICE_H
