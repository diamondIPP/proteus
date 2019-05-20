#pragma once

#include <vector>

#include "loop/analyzer.h"
#include "mechanics/geometry.h"
#include "utils/definitions.h"

class TDirectory;
class TH1D;
class TH2D;

namespace proteus {

class Device;

/** Global occupancy histograms for all sensors in the device. */
class GlobalOccupancy : public Analyzer {
public:
  GlobalOccupancy(TDirectory* dir, const Device& device);

  std::string name() const;
  void execute(const Event& event);

private:
  struct SensorHists {
    Index id = kInvalidIndex;
    TH2D* clustersXY = nullptr;
    TH1D* clustersT = nullptr;
  };

  const Geometry& m_geo;
  std::vector<SensorHists> m_sensorHists;
};

} // namespace proteus
