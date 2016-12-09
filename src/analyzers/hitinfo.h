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
          unsigned int timeBins = 16,
          unsigned int valueBins = 16);

  void processEvent(const Storage::Event* event);
  void postProcessing();

private:
  struct Hists {
    TH2D* pixels;
    TH1D* value;
    TH1D* time;
    TH2D* timeMap;
    TH2D* valueMap;
  };

  std::vector<Hists> m_hists;
};

} // namespace Analyzers

#endif // PT_HITINFO_H
