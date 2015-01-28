#ifndef OCCUPANCY_H_
#define OCCUPANCY_H_

#include <vector>

#include <Rtypes.h>
#include <TH2D.h>
#include <TH1D.h>
#include <TDirectory.h>

#include "singleanalyzer.h"

namespace Storage { class Event; }
namespace Mechanics { class Device; }

namespace Analyzers {
  
  class Occupancy : public SingleAnalyzer {
  public:
    Occupancy(const Mechanics::Device* device,
	      TDirectory* dir,
	      const char* suffix = "");
    
    void processEvent(const Storage::Event* event);
    void postProcessing();
    
    TH1D* getOccDistribution();
    TH2D* getHitOcc(unsigned int nsensor);
    
  public:
    /* Total number of hits in Device. For the telescope,
       this is the sum of hits in all planes. */
    ULong64_t totalHitOccupancy; 
    
  private:
    std::vector<TH2D*> _hitOcc;
    std::vector<TH2D*> _clusterOcc;
    std::vector<TH2D*> _clusterOccXZ;
    std::vector<TH2D*> _clusterOccYZ;
    std::vector<TH2D*> _clusterOccPix;
    TH1D* _occDistribution;

  }; // end of class
  
} // end of namespace

#endif // HITOCCUPANCY_H
