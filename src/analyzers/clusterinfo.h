#ifndef PT_CLUSTERINFO_H
#define PT_CLUSTERINFO_H

#include <vector>

#include "singleanalyzer.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Storage {
class Cluster;
class Event;
}
namespace Mechanics {
class Device;
}

namespace Analyzers {

class ClusterInfo : public SingleAnalyzer {
public:
  ClusterInfo(const Mechanics::Device* device,
              TDirectory* dir,
              const char* suffix = "",
              /* Histogram options */
              const int sizeMax = 8,
              const int timeMax = 32,
              const int valueMax = 32,
              const int binsUncertainty = 32);

  void processEvent(const Storage::Event* event);
  void postProcessing();

private:
  struct ClusterHists {
    TH1D* size;
    TH2D* sizeSizeCol;
    TH2D* sizeSizeRow;
    TH2D* sizeColSizeRow;
    TH1D* value;
    TH2D* sizeValue;
    TH1D* uncertaintyU;
    TH1D* uncertaintyV;
    TH2D* sizeHitValue;
    TH2D* sizeHitTime;
    TH2D* hitValueHitTime;

    void fill(const Storage::Cluster& cluster);
  };
  struct SensorHists {
    ClusterHists whole;
    std::vector<ClusterHists> regions;

    void fill(const Storage::Cluster& cluster);
  };

  std::vector<SensorHists> m_hists;
};

} // namespace Analyzers

#endif // PT_CLUSTERINFO_H
