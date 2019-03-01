#ifndef PT_CLUSTERS_H
#define PT_CLUSTERS_H

#include <vector>

#include "loop/analyzer.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Storage {
class SensorEvent;
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
                 const int sizeMax,
                 const int binsUncertainty);

  void execute(const Storage::SensorEvent& sensorEvent);
  void finalize();

private:
  struct AreaHists {
    TH1D* timestamp;
    TH1D* value;
    TH1D* size;
    TH2D* sizeTimestamp;
    TH2D* sizeValue;
    TH2D* sizeSizeCol;
    TH2D* sizeSizeRow;
    TH2D* sizeColSizeRow;
    TH1D* uncertaintyU;
    TH1D* uncertaintyV;
    TH1D* uncertaintyTime;
    TH2D* sizeHitTimestamp;
    TH1D* hitTimedelta;
    TH2D* sizeHitTimedelta;
    TH2D* sizeHitValue;
  };

  TH1D* m_nClusters;
  TH1D* m_rate;
  TH2D* m_colRow;
  AreaHists m_whole;
  std::vector<AreaHists> m_regions;
};

/** Cluster histograms for all sensors in the device. */
class Clusters : public Loop::Analyzer {
public:
  Clusters(TDirectory* dir,
           const Mechanics::Device& device,
           const int sizeMax = 9,
           const int binsUncertainty = 32);

  std::string name() const;
  void execute(const Storage::Event& event);
  void finalize();

private:
  std::vector<SensorClusters> m_sensors;
};

} // namespace Analyzers

#endif // PT_CLUSTERINFO_H
