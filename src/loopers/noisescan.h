#ifndef NOISESCAN_H
#define NOISESCAN_H

#include "looper.h"

namespace Storage { class StorageIO; }
namespace Mechanics { class Device; }
namespace Processors { class ClusterMaker; }
namespace Processors { class TrackMaker; }

namespace Loopers {
  
  class NoiseScan : public Looper
  {
  private:
    Mechanics::Device* _refDevice;
    double _maxFactor;
    double _maxOccupancy;
    int _bottomLimitX;
    int _upperLimitX;
    int _bottomLimitY;
    int _upperLimitY;
    
  public:
    NoiseScan(/* Use if you need mechanics (noise mask, pixel arrangement ...) */
	      Mechanics::Device* refDevice,
	      /* These arguments are needed to be passed to the base looper class */
	      Storage::StorageIO* refInput,
	      ULong64_t startEvent = 0,
	      ULong64_t numEvents = 0,
	      unsigned int eventSkip = 1);
    
    void loop();
    
    void setMaxFactor(double maxFactor);
    void setMaxOccupancy(double maxOccupancy);
    void setBottomLimitX(unsigned int bottomLimitX);
    void setUpperLimitX(unsigned int upperLimitX);
    void setBottomLimitY(unsigned int bottomLimitY);
    void setUpperLimitY(unsigned int upperLimitY);
    
    void print();

  }; // end of class
  
} // end of namespace 

#endif // EXAMPLELOOPER_H
