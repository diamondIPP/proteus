#ifndef PT_HITINFO_H
#define PT_HITINFO_H

#include <vector>

#include "analyzer.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Mechanics {
class Device;
class Sensor;
} // namespace Mechanics
namespace Storage {
class Plane;
}

namespace Analyzers {

/** Hit histograms for a single sensor. */
class SensorHitInfo {
public:
  SensorHitInfo(TDirectory* dir,
                const Mechanics::Sensor& sensor,
                const int timeMax,
                const int valueMax);

  void analyze(const Storage::Plane& sensorEvent);
  void finalize();

private:
  struct RegionHists {
    TH1D* time;
    TH1D* value;
  };

  TH1D* m_nHits;
  TH1D* m_rate;
  TH2D* m_pos;
  TH1D* m_time;
  TH1D* m_value;
  TH2D* m_meanTimeMap;
  TH2D* m_meanValueMap;
  std::vector<RegionHists> m_regions;
};

/** Hit histograms for all sensors in the device. */
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
  std::vector<SensorHitInfo> m_sensors;
};

} // namespace Analyzers

#endif // PT_HITINFO_H
