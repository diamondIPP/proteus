/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-12
 */

#ifndef PT_DISTANCES_H
#define PT_DISTANCES_H

#include <string>

#include "loop/analyzer.h"
#include "utils/definitions.h"

class TDirectory;
class TH1D;

namespace Mechanics {
class Sensor;
}

namespace Analyzers {

class Distances : public Loop::Analyzer {
public:
  Distances(TDirectory* dir, const Mechanics::Sensor& sensor);

  std::string name() const;
  void execute(const Storage::Event& event);

private:
  struct Hists {
    TH1D* deltaU = nullptr;
    TH1D* deltaV = nullptr;
    TH1D* deltaS = nullptr;
    TH1D* dist = nullptr;
  };

  Index m_sensorId = kInvalidIndex;
  Hists m_trackTrack;
  Hists m_trackCluster;
  Hists m_clusterCluster;
};

} // namespace Analyzers

#endif // PT_DISTANCES_H
