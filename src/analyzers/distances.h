/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-12
 */

#ifndef PT_DISTANCES_H
#define PT_DISTANCES_H

#include <string>

#include "analyzers/analyzer.h"
#include "utils/definitions.h"

class TDirectory;
class TH1D;

namespace Mechanics {
class Device;
}

namespace Analyzers {

class Distances : public Analyzer {
public:
  Distances(const Mechanics::Device& device, Index sensorId, TDirectory* dir);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  struct Hists {
    TH1D* deltaU;
    TH1D* deltaV;
    TH1D* dist;
    TH1D* d2;

    Hists() = default;
    Hists(TDirectory* dir,
          std::string prefix,
          double rangeDist,
          double rangeD2,
          int numBins);
    void fill(const XYVector& delta);
    void fill(const XYVector& delta, const SymMatrix2& cov);
  };

  Index m_sensorId;
  Hists m_trackTrack;
  Hists m_trackCluster;
  Hists m_match;
};

} // namespace Analyzers

#endif // PT_DISTANCES_H
