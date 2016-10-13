#include "singleanalyzer.h"

#include <iostream>
#include <stdexcept>
#include <sstream>

#include "../storage/event.h"
#include "../mechanics/device.h"

Analyzers::SingleAnalyzer::SingleAnalyzer(const Mechanics::Device* device,
					  TDirectory* dir,
					  const char* nameSuffix,
					  const char *analyzerName) :
  BaseAnalyzer(dir,nameSuffix,analyzerName),
  _device(device)
{ }


Analyzers::SingleAnalyzer::~SingleAnalyzer(){
}

void Analyzers::SingleAnalyzer::validSensor(unsigned int nsensor) {
  if(nsensor >= _device->getNumSensors())
    throw std::runtime_error("SingleAnalyzer: requested sensor exceeds range");
}

void Analyzers::SingleAnalyzer::eventDeviceAgree(const Storage::Event* event) {

  if (event->getNumPlanes() != _device->getNumSensors()){
    std::string err("SingleAnalyzer: event (");
    std::stringstream ss;
    ss << event->getNumPlanes() << ") vs device (" << _device->getNumSensors();
    err+=ss.str()+") plane mis-match";
    throw std::runtime_error(err);
  }
}

void Analyzers::SingleAnalyzer::print() const {
  std::cout << printStr() << std::endl;
}

const std::string Analyzers::SingleAnalyzer::printStr() const {
  std::ostringstream out;
  out << BaseAnalyzer::printStr();
  return out.str();
}
