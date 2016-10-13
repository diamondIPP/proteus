#ifndef OCCUPANCY_H_
#define OCCUPANCY_H_

#include <cstdint>
#include <vector>

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
	      const char* suffix="");

    void processEvent(const Storage::Event* event);

    /** Creates and fills the 1D occupancy histograms.*/
    void postProcessing();

    /** Returns Hit occupancy 2D-map for given sensor. */
    TH2D* getHitOcc(unsigned int nsensor);

    /** Returns Hit occupancy 1D-dist for given sensor.
	postProcessing() must have been called beforehand. */
    TH1D* getHitOccDist(unsigned int nsensors);

    /** Returns total number of hits for given sensor. */
    ULong64_t getTotalHitOccupancy(unsigned int sensor);
    
  private:
    void bookHistos(TDirectory *plotdir);
    
  private:
    uint64_t m_numEvents;
    std::vector<uint64_t> _totalHitOccupancy; /*!< Total number of hits in each plane
						 (sum of hits for all events). */

    std::vector<TH2D*> _hitOcc; //!< pixel hitmaps
    std::vector<TH2D*> _clusteredHitOcc; //!< clustered hit positions
    std::vector<TH2D*> _clusterOcc; //!< cluster positions (XY) 2D-histos.
    std::vector<TH1D*> _occDist; //!< Occupancy 1D-distributions (number of hits/trigger).
    
  }; // end of class
  
} // end of namespace

#endif // HITOCCUPANCY_H
