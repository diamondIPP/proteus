#ifndef PT_OCCUPANCY_H
#define PT_OCCUPANCY_H

#include <cstdint>
#include <vector>

#include "analyzers/analyzer.h"
#include "utils/definitions.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Mechanics {
class Device;
class Sensor;
}

namespace Analyzers {

class Occupancy : public Analyzer {
public:
  Occupancy(const Mechanics::Device* device,
            TDirectory* dir);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

  /** Returns Hit occupancy 2D-map for given sensor. */
  TH2D* getHitOcc(Index isensor);
  /** Returns Hit occupancy 1D-dist for given sensor.
      postProcessing() must have been called beforehand. */
  TH1D* getHitOccDist(Index isensor);
  /** Returns total number of hits for given sensor. */
  uint64_t getTotalHitOccupancy(Index isensor);

private:
  struct Hists {
    TH2D* hitMap;
    TH1D* hitOccupancyDist;
    TH2D* clusteredHitMap;
    TH1D* clusteredHitOccupancyDist;
    TH2D* clusterMap;
    TH1D* clusterOccupancyDist;

    Hists(const Mechanics::Sensor& sensor,
          TDirectory* dir,
          const int occupancyBins = 128);
  };

  uint64_t m_numEvents;
  std::vector<Hists> m_hists;
};

} // namespace Analyzers

#endif // PT_OCCUPANCY_H
