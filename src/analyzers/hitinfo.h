#ifndef PT_HITINFO_H
#define PT_HITINFO_H

#include <vector>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "singleanalyzer.h"

namespace Storage {
class Event;
}
namespace Mechanics {
class Device;
}

namespace Analyzers {

class HitInfo : public SingleAnalyzer {
public:
  HitInfo(const Mechanics::Device* device,
          TDirectory* dir,
          const char* suffix = "",
          /* Histogram options */
          unsigned int lvl1Bins = 16,
          unsigned int totBins = 16);

  void processEvent(const Storage::Event* event);
  void postProcessing();

private:
  std::vector<TH1D*> _lvl1;
  std::vector<TH1D*> _tot;
  std::vector<TH2D*> _totMap;
  std::vector<TH2D*> _timingMap;
  std::vector<TH2D*> _MapCnt;

  unsigned int _lvl1Bins;
  unsigned int _totBins;
};

} // namespace Analyzers

#endif // PT_HITINFO_H
