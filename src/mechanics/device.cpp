#include "device.h"

#include <cassert>
#include <vector>
#include <iostream>
#include <string.h>
#include <string>
#include <algorithm>

#include <Rtypes.h>

#include "utils/definitions.h"
#include "sensor.h"
#include "noisemask.h"
#include "alignment.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

using std::cout;
using std::endl;

//=========================================================
Mechanics::Device::Device(const char* name,
			  const char* alignmentName,
			  const char* noiseMaskName,
			  double clockRate,
			  unsigned int readOutWindow,
			  const char* spaceUnit,
			  const char* timeUnit) :
  _name(name),
  _clockRate(clockRate),
  _readOutWindow(readOutWindow),
  _spaceUnit(spaceUnit),
  _timeUnit(timeUnit),
  _beamSlopeX(0),
  _beamSlopeY(0),
  _timeStart(0),
  _timeEnd(0),
  _syncRatio(0),
  _numSensors(0),
  _noiseMask(0),
  _alignment(0)
{
  if (strlen(alignmentName)) {
    _alignment = new Alignment();
	  _alignment->readFile(alignmentName);
    setAlignment(*_alignment);
  }

  if (strlen(noiseMaskName))
    _noiseMask = new NoiseMask(noiseMaskName);

  std::replace(_timeUnit.begin(), _timeUnit.end(), '\\', '#');
  std::replace(_spaceUnit.begin(), _spaceUnit.end(), '\\', '#');
}

//=========================================================
Mechanics::Device::~Device(){
  for(unsigned int nsensor=0; nsensor<_numSensors; nsensor++)
    delete _sensors.at(nsensor);
  if (_noiseMask) delete _noiseMask;
  if (_alignment) delete _alignment;
}

//=========================================================
void Mechanics::Device::addSensor(Mechanics::Sensor* sensor){
  assert(sensor && "Device: can't add a null sensor");
  
  if (_numSensors > 0 && getSensor(_numSensors-1)->getOffZ() > sensor->getOffZ())
    throw "[Device::addSensor] sensors must be added in order of increazing Z position";
  _sensors.push_back(sensor);
	_sensors.back()->setNoisyPixels(*_noiseMask, _numSensors);
  _sensorMask.push_back(false);
  _numSensors++;
}

//=========================================================
void Mechanics::Device::addMaskedSensor() {
  _sensorMask.push_back(true);
}

//=========================================================
double Mechanics::Device::tsToTime(ULong64_t timeStamp) const {
  return (double)((timeStamp - getTimeStart()) / (double)getClockRate());
}

//=========================================================
Mechanics::Sensor* Mechanics::Device::getSensor(unsigned int n) const {
  assert(n < _numSensors && "Device: sensor index outside range");
  return _sensors.at(n);
}

//=========================================================
unsigned int Mechanics::Device::getNumPixels() const
{
  unsigned int numPixels = 0;
  for (unsigned int nsens = 0; nsens < getNumSensors(); nsens++)
    numPixels += getSensor(nsens)->getNumPixels();
  return numPixels;
}

//=========================================================
void Mechanics::Device::print() const {
  cout << "\nDEVICE:\n"
       << "  Name: '" << _name << "'\n"
       << "  Clock rate: " << _clockRate << "\n"
       << "  Read out window: " << _readOutWindow << "\n"
       << "  Sensors: " << _numSensors << "\n"
       << "  Alignment: " << _alignment->m_path << "\n"
       << "  Noisemask: " << _noiseMask->getFileName() << endl;
  
  for (unsigned int nsens=0; nsens<getNumSensors(); nsens++)
    getSensor(nsens)->print();
}

void Mechanics::Device::setAlignment(const Alignment& alignment)
{
  for (Index sensorId = 0; sensorId < getNumSensors(); ++sensorId) {
    Sensor* sensor = getSensor(sensorId);
    sensor->setLocalToGlobal(alignment.getLocalToGlobal(sensorId));
  }

  _beamSlopeX = alignment.m_beamSlopeX;
  _beamSlopeY = alignment.m_beamSlopeY;
  _syncRatio = alignment.m_syncRatio;
}
