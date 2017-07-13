#ifndef PT_CLUSTERINFO_H
#define PT_CLUSTERINFO_H

#include <vector>

#include "analyzer.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Storage {
class Cluster;
}
namespace Mechanics {
class Device;
}

namespace Analyzers {

class ClusterInfo : public Analyzer {
public:
  ClusterInfo(const Mechanics::Device* device,
              TDirectory* dir,
              /* Histogram options */
              const int sizeMax = 8,
              const int timeMax = 32,
              const int valueMax = 32,
              const int binsUncertainty = 32);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  struct ClusterHists {
    TH2D* pos;
    TH1D* value;
    TH1D* size;
    TH2D* sizeSizeCol;
    TH2D* sizeSizeRow;
    TH2D* sizeColSizeRow;
    TH2D* sizeValue;
    TH1D* uncertaintyU;
    TH1D* uncertaintyV;
    TH2D* hitPos;
    TH2D* sizeHitValue;
    TH2D* sizeHitTime;
    TH2D* hitValueHitTime;

    void fill(const Storage::Cluster& cluster);
  };
  struct SensorHists {
    TH1D* nClusters;
    TH1D* rate;
    ClusterHists whole;
    std::vector<ClusterHists> regions;

    void fill(const Storage::Cluster& cluster);
  };

  std::vector<SensorHists> m_hists;
};

} // namespace Analyzers

#endif // PT_CLUSTERINFO_H
