#ifndef CLUSTERINFO_H
#define CLUSTERINFO_H

#include <vector>

#include <TH2D.h>
#include <TH1D.h>
#include <TDirectory.h>

#include "singleanalyzer.h"

namespace Storage { class Event; }
namespace Mechanics { class Device; }

namespace Analyzers {
  
  class ClusterInfo : public SingleAnalyzer {
  public:
    ClusterInfo(const Mechanics::Device* device,
		TDirectory* dir,
		const char* suffix = "",
		/* Histogram options */
		unsigned int totBins = 20,
		unsigned int maxClusterSize = 5);
    
    void processEvent(const Storage::Event* event);
    void postProcessing();
    
  private:
    struct ClusterHists {
      TH1D* size;
      TH2D* sizeColSize;
      TH2D* sizeRowSize;
      TH2D* sizeRowSizeCol;
      TH1D* uncCol;
      TH1D* uncRow;
    };

    std::vector<ClusterHists> m_hists;
    std::vector<TH1D*> _tot;
    std::vector<TH2D*> _totSize;
    std::vector<TH1D*> _clustersVsTime;
    std::vector<TH1D*> _totVsTime;
    std::vector<TH2D*> _timingVsClusterSize;
    std::vector<TH2D*> _timingVsHitValue;
    unsigned int _totBins;

  }; // end of class

} // end of namespace

#endif // CLUSTERINFO_H
