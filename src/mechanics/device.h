#ifndef DEVICE_H
#define DEVICE_H

#include <vector>
#include <math.h>
#include <string>

#include <Rtypes.h>

namespace Mechanics {

  class Sensor;
  class NoiseMask;
  class Alignment;
  
  class Device {
  public:
    Device(const char* name,
	   const char* alignmentName="",
	   const char* noiseMaskName="",
	   double clockRate=0,
	   unsigned int readOutWindow=0,
	   const char* spaceUnit="",
	   const char* timeUnit="");

    ~Device();
    
    void addSensor(Sensor* sensor);
    void addMaskedSensor();
    double tsToTime(ULong64_t timeStamp) const;
    
    void setBeamSlopeX(double rotation) { _beamSlopeX = rotation; }
    void setBeamSlopeY(double rotation) { _beamSlopeY = rotation; }
    void setTimeStart(ULong64_t timeStamp) { _timeStart = timeStamp; }
    void setTimeEnd(ULong64_t timeStamp) { _timeEnd = timeStamp; }
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
    ULong64_t getTimeStart() const { return _timeStart; }
    ULong64_t getTimeEnd() const { return _timeEnd; }
    double getSyncRatio() const { return _syncRatio; }
    
    NoiseMask* getNoiseMask() {  return _noiseMask; }
    Alignment* getAlignment() { return _alignment; }

    void print() const;

  private:
    void setAlignment(const Alignment& alignment);

    std::string _name;
    double _clockRate;
    unsigned int _readOutWindow;
    std::string _spaceUnit;
    std::string _timeUnit;
    double _beamSlopeX;
    double _beamSlopeY;
    ULong64_t _timeStart;
    ULong64_t _timeEnd;
    double _syncRatio;
    
    unsigned int _numSensors;
    std::vector<Sensor*> _sensors;
    std::vector<bool> _sensorMask;
    
    NoiseMask* _noiseMask;
    Alignment* _alignment;
    
  }; // end of class
} // end of namespace Mechanics

#endif // DEVICE_H
