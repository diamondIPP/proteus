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
            const double reducedChi2Max = 10,
            const double slopeMax = 0.01);

  void processEvent(const Storage::Event* event);
  void postProcessing();

private:
  TH1D* m_numClusters;
  TH1D* m_reducedChi2;
  TH2D* m_offsetXY;
  TH1D* m_offsetX;
  TH1D* m_offsetY;
  TH2D* m_slopeXY;
  TH1D* m_slopeX;
  TH1D* m_slopeY;
};

} // namespace Analyzers

#endif // PT_TRACKINFO_H
