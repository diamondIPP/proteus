#ifndef PT_CLUSTERS_H
#define PT_CLUSTERS_H

#include <vector>

#include "analyzer.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Storage {
class Plane;
}
namespace Mechanics {
class Device;
class Sensor;
} // namespace Mechanics

namespace Analyzers {

/** Cluster histograms for a single sensor. */
class SensorClusters {
public:
  SensorClusters(TDirectory* dir,
                 const Mechanics::Sensor& sensor,
                 const int timeMax,
                 const int valueMax,
                 const int sizeMax,
                 const int binsUncertainty);

  void analyze(const Storage::Plane& sensorEvent);
  void finalize();

private:
  struct AreaHists {
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
    TH2D* sizeHitTime;
    TH2D* sizeHitValue;
    TH2D* hitValueHitTime;
  };

  TH1D* m_nClusters;
  TH1D* m_rate;
  AreaHists m_whole;
  std::vector<AreaHists> m_regions;
};

/** Cluster histograms for all sensors in the device. */
class Clusters : public Analyzer {
public:
  Clusters(TDirectory* dir,
           const Mechanics::Device& device,
           /* Histogram options */
           const int sizeMax = 8,
           const int timeMax = 32,
           const int valueMax = 32,
           const int binsUncertainty = 32);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  std::vector<SensorClusters> m_sensors;
};

} // namespace Analyzers

#endif // PT_CLUSTERINFO_H
