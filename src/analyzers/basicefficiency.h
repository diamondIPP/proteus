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

/** Basic efficiency calculation using tracks and matched clusters. */
class BasicEfficiency : public Analyzer {
public:
  /**
   * \param sensor Sensor for which efficiencies should be calculated
   * \param dir Histogram output directory
   * \param increaseArea Extend histograms beyond the nominal sensor edge
   * \param maskedPixelRange Remove tracks around masked pixels, 0 to disable
   * \param inPixelPeriod Folding period in number of pixels
   * \param inPixelBinsMin Minimum number of bins along the smaller direction
   */
  BasicEfficiency(const Mechanics::Sensor& sensor,
                  TDirectory* dir,
                  int increaseArea = 2,
                  int maskedPixelRange = 1,
                  int inPixelPeriod = 2,
                  int inPixelBinsMin = 32);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  using Area = Utils::Box<2, double>;
  struct Hists {
    Area areaFullPixel; // in pixel coordinates
    Area areaFoldLocal; // in local coordinates
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
    TH2D* inPixTotal;
    TH2D* inPixPass;
    TH2D* inPixFail;
    TH2D* inPixEff;

    Hists() = default;
    Hists(const std::string& prefix,
          Area fullPixel,
          Area foldLocal,
          int foldBinsU,
          int foldBinsV,
          TDirectory* dir);
    void fill(const Storage::TrackState& state, const XYPoint& posPixel);
    void finalize();
  };

  const Mechanics::Sensor& m_sensor;
  Utils::DenseMask m_mask;
  Hists m_whole;
  std::vector<Hists> m_regions;
};

} // namespace Analyzers

#endif // PT_BASICEFFICIENCY_H
