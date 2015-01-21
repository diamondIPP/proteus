#ifndef EFFICIENCY_H
#define EFFICIENCY_H

#include <vector>

#include <TH2D.h>
#include <TH1D.h>
#include <TEfficiency.h>
#include <TDirectory.h>

#include "dualanalyzer.h"

namespace Storage { class Event; }
namespace Mechanics { class Device; }

namespace Analyzers {
  
  class Efficiency : public DualAnalyzer
  {
  public:
    Efficiency(const Mechanics::Device* refDevice,
	       const Mechanics::Device* dutDevice,
	       TDirectory* dir = 0,
	       const char* suffix = "",
	       /* Additional parameters with default values */
	       int relativeToSensor = -1, // Only consider events where this sensor has a match
	       int pix_x_min = 1,
	       int pix_x_max = 12,
	       int pix_y_min = 81,
	       int pix_y_max = 94,
	       /* Histogram options */
	       unsigned int rebinX = 1, // This many pixels in X are grouped
	       unsigned int rebinY = 1,
	       unsigned int pixBinsX = 20,
	       unsigned int pixBinsY = 20);
    
    void processEvent(const Storage::Event* refEvent,
		      const Storage::Event* dutDevent);
    
    void postProcessing();
    
  private:
    int _relativeToSensor;
    int _pix_x_min; // minimum pixel (x) to be considered for in-pixel results
    int _pix_x_max; // maximum pixel (x) to be considered for in-pixel results
    int _pix_y_min; // minimum pixel (y) to be considered for in-pixel results
    int _pix_y_max; // maximum pixel (y) to be considered for in-pixel results

    // vector histograms 
    std::vector<TEfficiency*> _efficiencyMap;
    std::vector<TH1D*> _efficiencyDistribution;
    std::vector<TH1D*> _matchedTracks;
    std::vector<TH1D*> _matchpositionX;
    std::vector<TH1D*> _matchpositionY;
    std::vector<TEfficiency*> _efficiencyTime;
    std::vector<TEfficiency*> _inPixelEfficiency;
    std::vector<TH2D*> _inPixelTiming;
    std::vector<TH2D*> _inPixelCounting;
    
  }; // end of class
  
} // end of namespace

#endif // EFFICIENCY_H
