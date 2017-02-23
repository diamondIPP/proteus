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
          const int timeMax = 16,
          const int valueMax = 16);

  void processEvent(const Storage::Event* event);
  void postProcessing();

private:
  struct RegionHists {
    TH1D* time;
    TH1D* value;
  };
  struct SensorHists {
    TH2D* hitMap;
    TH2D* meanTimeMap;
    TH2D* meanValueMap;
    RegionHists whole;
    std::vector<RegionHists> regions;
  };

  std::vector<SensorHists> m_hists;
};

} // namespace Analyzers

#endif // PT_HITINFO_H
