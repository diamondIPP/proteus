#ifndef SINGLEANALYZER_H
#define SINGLEANALYZER_H

#include "baseanalyzer.h"

#include <string>

namespace Storage { class Event; }
namespace Mechanics { class Device; }

namespace Analyzers {
  
  class SingleAnalyzer : public BaseAnalyzer {
  protected:
    SingleAnalyzer(const Mechanics::Device* device,
		   TDirectory* dir=0,
		   const char* nameSuffix="",
		   const char* analyzerName="UNNAMED");
    
    void validSensor(unsigned int nsensor);
    void eventDeviceAgree(const Storage::Event* event);

  public:
    virtual ~SingleAnalyzer();
    
    // This behaviour NEEDS to be implemented in the derived classes
    virtual void processEvent(const Storage::Event* event) = 0;
    virtual void postProcessing() = 0;

    virtual void print() const;
    virtual const std::string printStr() const;

  protected:
    const Mechanics::Device* _device;
      
  }; // end of class
  
} // end of namespace

#endif // SINGLEANALYZER_H
