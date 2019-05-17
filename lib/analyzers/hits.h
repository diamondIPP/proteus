#ifndef PT_HITS_H
#define PT_HITS_H

#include <vector>

#include "loop/analyzer.h"

class TDirectory;
class TH1D;
class TH2D;

namespace proteus {

class Device;
class Sensor;
class SensorEvent;

/** Hit histograms for a single sensor. */
class SensorHits {
public:
  SensorHits(TDirectory* dir, const Sensor& sensor);

  void execute(const SensorEvent& sensorEvent);
  void finalize();

private:
  struct RegionHists {
    TH1D* timestamp;
    TH1D* value;
    TH2D* valueTimestamp;
  };

  TH1D* m_nHits;
  TH1D* m_rate;
  TH2D* m_colRow;
  TH1D* m_timestamp;
  TH1D* m_value;
  TH2D* m_valueTimestamp;
  TH2D* m_meanTimestampMap;
  TH2D* m_meanValueMap;
  std::vector<RegionHists> m_regions;
};

/** Hit histograms for all sensors in the device. */
class Hits : public Analyzer {
public:
  Hits(TDirectory* dir, const Device& device);

  std::string name() const;
  void execute(const Event& event);
  void finalize();

private:
  std::vector<SensorHits> m_sensors;
};

} // namespace proteus

#endif // PT_HITS_H
