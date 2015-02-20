#include "dualanalyzer.h"

#include <iostream>
#include <sstream>

#include "../storage/event.h"
#include "../mechanics/device.h"

Analyzers::DualAnalyzer::DualAnalyzer(const Mechanics::Device* refDevice,
				      const Mechanics::Device* dutDevice,
				      TDirectory* dir,
				      const char* nameSuffix,
				      const char *analyzerName) :
  BaseAnalyzer(dir,nameSuffix,analyzerName),
  _refDevice(refDevice),
  _dutDevice(dutDevice)
{}

Analyzers::DualAnalyzer::~DualAnalyzer(){
}

void Analyzers::DualAnalyzer::validRefSensor(unsigned int nsensor){
  if (nsensor >= _refDevice->getNumSensors())
    throw "DualAnalyzer: requested sensor exceeds range";
}

void Analyzers::DualAnalyzer::validDutSensor(unsigned int nsensor){
  if(nsensor >= _dutDevice->getNumSensors())
    throw "DualAnalyzer: requested sensor exceeds range";
}

void Analyzers::DualAnalyzer::eventDeviceAgree(const Storage::Event* refEvent,
					       const Storage::Event* dutEvent){
  if (refEvent->getNumPlanes() != _refDevice->getNumSensors() ||
      dutEvent->getNumPlanes() != _dutDevice->getNumSensors())
    throw "DualAnalyzer: event / device plane mis-match";
}

void Analyzers::DualAnalyzer::print() const {
  std::cout << printStr() << std::endl;
}

const std::string Analyzers::DualAnalyzer::printStr() const {
  std::ostringstream out;
  out << BaseAnalyzer::printStr();
  return out.str();
}
