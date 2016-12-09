#ifndef PT_TRACKINFO_H
#define PT_TRACKINFO_H

#include <vector>

#include "singleanalyzer.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Storage {
class Event;
}
namespace Mechanics {
class Device;
}

namespace Analyzers {

class TrackInfo : public SingleAnalyzer {
public:
  TrackInfo(const Mechanics::Device* device,
            TDirectory* dir,
            const char* suffix = "",
            /* Histogram options */
            double resWidth = 1E-2, // Widht of track resolution histos
            double maxSploe = 1E-2, // Maximal slope for track slope histos
            double increaseArea =
                1.2); // Make origins plot larger than sensor by this factor

  void processEvent(const Storage::Event* event);
  void postProcessing();

private:
  TH2D* _origins;
  TH1D* _originsX;
  TH1D* _originsY;
  TH1D* _slopesX;
  TH1D* _slopesY;
  TH1D* _chi2;
  TH1D* _numClusters;
  TH1D* _resoX;
  TH1D* _resoY;

  std::vector<TH1D*> _resX;
  std::vector<TH1D*> _resY;
};

} // namespace Analyzers

#endif // PT_TRACKINFO_H
