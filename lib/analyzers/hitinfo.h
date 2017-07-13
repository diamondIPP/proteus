#ifndef PT_HITINFO_H
#define PT_HITINFO_H

#include <vector>

#include "analyzer.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Mechanics {
class Device;
}

namespace Analyzers {

class HitInfo : public Analyzer {
public:
  HitInfo(const Mechanics::Device* device,
          TDirectory* dir,
          /* Histogram options */
          const int timeMax = 16,
          const int valueMax = 16);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  struct RegionHists {
    TH1D* time;
    TH1D* value;
  };
  struct SensorHists {
    TH1D* nHits;
    TH1D* rate;
    TH2D* pos;
    TH1D* time;
    TH1D* value;
    TH2D* meanTimeMap;
    TH2D* meanValueMap;
    std::vector<RegionHists> regions;
  };

  std::vector<SensorHists> m_hists;
};

} // namespace Analyzers

#endif // PT_HITINFO_H
