/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02-16
 */

#ifndef PT_BASICEFFICIENCY_H
#define PT_BASICEFFICIENCY_H

#include <vector>

#include "analyzers/analyzer.h"
#include "utils/definitions.h"
#include "utils/densemask.h"
#include "utils/interval.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Mechanics {
class Sensor;
}
namespace Storage {
class TrackState;
}

namespace Analyzers {

/** Basic efficiency calculation using tracks and matched clusters.
 *
 * Computer sensor and in-pixel efficiency maps and projections. Two-dimensional
 * efficiency maps are calculated with additional eges to include also tracks
 * that are matched to a cluster but are located outside the region of interest.
 * The per-pixel efficiency distribution is calculated without these edges
 * pixels.
 */
class BasicEfficiency : public Analyzer {
public:
  /**
   * \param sensor Sensor for which efficiencies should be calculated
   * \param dir Histogram output directory
   * \param maskedPixelRange Remove tracks around masked pixels, 0 to disable
   * \param increaseArea Extend histograms beyond the nominal sensor edge
   * \param inPixelPeriod Folding period in number of pixels
   * \param inPixelBinsMin Minimum number of bins along the smaller direction
   * \param efficiencyDistBins Number of bins in the efficiency distribution
   */
  BasicEfficiency(const Mechanics::Sensor& sensor,
                  TDirectory* dir,
                  int maskedPixelRange = 1,
                  int increaseArea = 2,
                  int inPixelPeriod = 2,
                  int inPixelBinsMin = 32,
                  int efficiencyDistBins = 128);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  using Area = Utils::Box<2, double>;
  struct Hists {
    Area areaPixel; // region-of-interest area + edge bins
    Area roiPixel;  // only the region-of-interest
    int edgeBins;   // how many bins are edges outside the region-of-interest
    TH2D* total;
    TH2D* pass;
    TH2D* fail;
    TH2D* eff;
    TH1D* effDist;
    TH1D* colTotal;
    TH1D* colPass;
    TH1D* colFail;
    TH1D* colEff;
    TH1D* rowTotal;
    TH1D* rowPass;
    TH1D* rowFail;
    TH1D* rowEff;
    Area inPixelAreaLocal; // in local coordinates
    TH2D* inPixTotal;
    TH2D* inPixPass;
    TH2D* inPixFail;
    TH2D* inPixEff;

    Hists() = default;
    Hists(const std::string& prefix,
          const Mechanics::Sensor& sensor,
          Area roi,
          int increaseArea,
          int inPixelPeriod,
          int inPixelBinsMin,
          int efficiencyDistBins,
          TDirectory* dir);
    void fill(const Storage::TrackState& state, const XYPoint& posPixel);
    void finalize();
  };

  const Mechanics::Sensor& m_sensor;
  Utils::DenseMask m_mask;
  Hists m_sensorHists;
  std::vector<Hists> m_regionsHists;
};

} // namespace Analyzers

#endif // PT_BASICEFFICIENCY_H
