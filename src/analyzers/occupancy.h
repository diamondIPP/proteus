#ifndef PT_OCCUPANCY_H
#define PT_OCCUPANCY_H

#include <cstdint>
#include <vector>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "analyzers/singleanalyzer.h"
#include "utils/definitions.h"

namespace Storage {
class Event;
}
namespace Mechanics {
class Device;
class Sensor;
}

namespace Analyzers {

class Occupancy : public SingleAnalyzer {
public:
  Occupancy(const Mechanics::Device* device,
            TDirectory* dir,
            const char* suffix = "");

  void processEvent(const Storage::Event* event);
  void postProcessing();

  /** Returns Hit occupancy 2D-map for given sensor. */
  TH2D* getHitOcc(Index isensor);
  /** Returns Hit occupancy 1D-dist for given sensor.
      postProcessing() must have been called beforehand. */
  TH1D* getHitOccDist(Index isensor);
  /** Returns total number of hits for given sensor. */
  uint64_t getTotalHitOccupancy(Index isensor);

private:
  struct Hists {
    TH2D* hits;
    TH2D* clusteredHits;
    TH2D* clusters;
    TH1D* pixelOccupancy;

    Hists(const Mechanics::Sensor& sensor,
          TDirectory* dir,
          const int binsPerClusterPixel = 3,
          const int binsOccupancy = 128);
  };

  uint64_t m_numEvents;
  std::vector<Hists> m_hists;
};

} // namespace Analyzers

#endif // PT_OCCUPANCY_H
