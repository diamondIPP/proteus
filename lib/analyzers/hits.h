#ifndef PT_HITS_H
#define PT_HITS_H

#include <vector>

#include "loop/analyzer.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Mechanics {
class Device;
class Sensor;
} // namespace Mechanics
namespace Storage {
class SensorEvent;
}

namespace Analyzers {

/** Hit histograms for a single sensor. */
class SensorHits {
public:
  SensorHits(TDirectory* dir, const Mechanics::Sensor& sensor);

  void execute(const Storage::SensorEvent& sensorEvent);
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
class Hits : public Loop::Analyzer {
public:
  Hits(TDirectory* dir, const Mechanics::Device& device);

  std::string name() const;
  void execute(const Storage::Event& event);
  void finalize();

private:
  std::vector<SensorHits> m_sensors;
};

} // namespace Analyzers

#endif // PT_HITS_H
