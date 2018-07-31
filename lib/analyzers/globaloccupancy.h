#ifndef PT_GLOBALOCCUPANCY_H
#define PT_GLOBALOCCUPANCY_H

#include <vector>

#include "loop/analyzer.h"
#include "mechanics/geometry.h"
#include "utils/definitions.h"

class TDirectory;
class TH2D;

namespace Mechanics {
class Device;
}
namespace Analyzers {

/** Global occupancy histograms for all sensors in the device. */
class GlobalOccupancy : public Loop::Analyzer {
public:
  GlobalOccupancy(TDirectory* dir, const Mechanics::Device& device);

  std::string name() const;
  void execute(const Storage::Event& event);

private:
  struct SensorHists {
    Mechanics::Plane plane;
    Index id = kInvalidIndex;
    TH2D* clusters_xy = nullptr;
  };

  std::vector<SensorHists> m_sensorHists;
};

} // namespace Analyzers

#endif // PT_GLOBALOCCUPANCY_H
