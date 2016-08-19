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

  void setBeamSlopeX(double rotation) { _beamSlopeX = rotation; }
  void setBeamSlopeY(double rotation) { _beamSlopeY = rotation; }
  void setTimeStart(uint64_t timeStamp) { _timeStart = timeStamp; }
  void setTimeEnd(uint64_t timeStamp) { _timeEnd = timeStamp; }
  void setSyncRatio(double ratio) { _syncRatio = ratio; }

  unsigned int getNumSensors() const { return _numSensors; }
  Sensor* getSensor(unsigned int n) const;

  unsigned int getNumPixels() const;

  const std::vector<bool>* getSensorMask() const { return &_sensorMask; }

  const char* getName() const { return _name.c_str(); }
  double getClockRate() const { return _clockRate; }
  unsigned int getReadOutWindow() const { return _readOutWindow; }
  const char* getSpaceUnit() const { return _spaceUnit.c_str(); }
  const char* getTimeUnit() const { return _timeUnit.c_str(); }
  double getBeamSlopeX() const { return _beamSlopeX; }
  double getBeamSlopeY() const { return _beamSlopeY; }
  uint64_t getTimeStart() const { return _timeStart; }
  uint64_t getTimeEnd() const { return _timeEnd; }
  double getSyncRatio() const { return _syncRatio; }

  NoiseMask* getNoiseMask() { return &_noiseMask; }
  Alignment* getAlignment() { return &_alignment; }

  void print() const;

private:
  std::string _name;
  double _clockRate;
  unsigned int _readOutWindow;
  std::string _spaceUnit;
  std::string _timeUnit;
  double _beamSlopeX;
  double _beamSlopeY;
  uint64_t _timeStart;
  uint64_t _timeEnd;
  double _syncRatio;

  unsigned int _numSensors;
  std::vector<Sensor*> _sensors;
  std::vector<bool> _sensorMask;

  Alignment _alignment;
  NoiseMask _noiseMask;
};

} // namespace Mechanics

#endif // DEVICE_H
