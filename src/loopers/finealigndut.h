#ifndef FINEALIGNDUT_H
#define FINEALIGNDUT_H

#include "looper.h"

namespace Storage { class StorageIO; }
namespace Mechanics { class Device; }
namespace Processors { class ClusterMaker; }
namespace Processors { class TrackMaker; }

namespace Loopers {
  
  class FineAlignDut : public Looper {
  public:
    FineAlignDut(/* Use if you need mechanics (noise mask, pixel arrangement ...) */
		 Mechanics::Device* refDevice,
		 Mechanics::Device* dutDevice,
		 /* Use if the looper needs to make clusters and/or tracks... */
		 Processors::ClusterMaker* clusterMaker,
		 Processors::TrackMaker* trackMaker,
		 /* These arguments are needed to be passed to the base looper class */
		 Storage::StorageIO* refInput,
		 Storage::StorageIO* dutInput,
		 ULong64_t startEvent = 0,
		 ULong64_t numEvents = 0,
		 unsigned int eventSkip = 1);

    void loop();
    
    void setNumIterations(unsigned int value);
    void setNumBinsY(unsigned int value);
    void setNumPixX(unsigned int value);
    void setBinsPerPix(double value);
    void setNumPixXBroad(unsigned int value);
    void setBinsPerPixBroad(double value);
    void setDisplayFits(bool value);
    void setRelaxation(double value);
    
    void print() const;
    
  private:
    Mechanics::Device* _refDevice;
    Mechanics::Device* _dutDevice;
    Processors::ClusterMaker* _clusterMaker;
    Processors::TrackMaker* _trackMaker;
    
    unsigned int _numIterations;
    unsigned int _numBinsY;
    unsigned int _numPixX;
    double _binsPerPix;
    unsigned int _numPixXBroad;
    double _binsPerPixBroad;
    bool _displayFits;
    double _relaxation;

  }; // end of class

} // end of namespace 

#endif // FINEALIGNDUT_H
