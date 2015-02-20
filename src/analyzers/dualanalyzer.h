#ifndef DUALANALYZER_H
#define DUALANALYZER_H

#include "baseanalyzer.h"

#include <string>

namespace Storage { class Event; }
namespace Mechanics { class Device; }

namespace Analyzers {
  
  class DualAnalyzer : public BaseAnalyzer {
  protected:
    DualAnalyzer(const Mechanics::Device* refDevice,
		 const Mechanics::Device* dutDevice,
		 TDirectory* dir=0,
		 const char* nameSuffix="",
		 const char* analyzerName="UNNAMED");
    
    void validRefSensor(unsigned int nsensor);
    void validDutSensor(unsigned int nsensor);
    void eventDeviceAgree(const Storage::Event* refEvent,
			  const Storage::Event* dutEvent);

  public:
    virtual ~DualAnalyzer();
    
    // This behaviour NEEDS to be implemented in the derived classes
    virtual void processEvent(const Storage::Event* refEvent,
			      const Storage::Event* dutEvent) = 0;
    
    virtual void postProcessing() = 0;

    virtual void print() const;
    virtual const std::string printStr() const;
    
  protected:
    const Mechanics::Device* _refDevice;
    const Mechanics::Device* _dutDevice;

  }; // end of class  
} // end of namespace

#endif // DUALANALYZER_H
