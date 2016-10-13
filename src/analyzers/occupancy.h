#ifndef PT_OCCUPANCY_H
#define PT_OCCUPANCY_H

#include <cstdint>
#include <vector>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "singleanalyzer.h"

namespace Storage {
class Event;
}
namespace Mechanics {
class Device;
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
  TH2D* getHitOcc(unsigned int nsensor);
  /** Returns Hit occupancy 1D-dist for given sensor.
      postProcessing() must have been called beforehand. */
  TH1D* getHitOccDist(unsigned int nsensors);
  /** Returns total number of hits for given sensor. */
  uint64_t getTotalHitOccupancy(unsigned int sensor);

private:
  void bookHistos(const Mechanics::Device& device, TDirectory* dir);

  uint64_t m_numEvents;
  std::vector<TH2D*> m_occHits;         //!< pixel hitmaps
  std::vector<TH2D*> m_occClustersHits; //!< clustered hit positions
  std::vector<TH2D*> m_occClusters;     //!< cluster positions (XY) 2D-histos.
  std::vector<TH1D*>
      m_occPixels; //!< Occupancy 1D-distributions (number of hits/trigger).
};

} // namespace Analyzers

#endif // PT_OCCUPANCY_H
