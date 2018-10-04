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
  /**
   * \param pixelRange Distance histogram range in number of pixels
   * \param d2Max Maximum weighted squared distance in histogram
   * \param bins Number of histogram bins
   */
  Distances(TDirectory* dir,
            const Mechanics::Sensor& sensor,
            const double d2Max = 10.0,
            const int bins = 256);

  std::string name() const;
  void execute(const Storage::Event& event);

private:
  struct Hists {
    TH1D* deltaU;
    TH1D* deltaV;
    TH1D* dist;
    TH1D* d2;

    Hists() = default;
    Hists(TDirectory* dir,
          const std::string& prefix,
          const double distMax,
          const double d2Max,
          const int bins);
    void fill(const Vector2& delta);
    void fill(const Vector2& delta, const SymMatrix2& cov);
  };

  Index m_sensorId;
  Hists m_trackTrack;
  Hists m_trackCluster;
  Hists m_clusterCluster;
};

} // namespace Analyzers

#endif // PT_DISTANCES_H
