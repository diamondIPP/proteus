/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02-16
 */

#ifndef PT_BASICEFFICIENCY_H
#define PT_BASICEFFICIENCY_H

#include "analyzers/analyzer.h"
#include "utils/definitions.h"
#include "utils/densemask.h"

class TDirectory;
class TH1D;
class TH2D;

namespace Mechanics {
class Sensor;
}

namespace Analyzers {

/** Basic efficiency calculation using tracks and matched clusters. */
class BasicEfficiency : public Analyzer {
public:
  /**
   * \param sensor Sensor for which efficiencies should be calculated
   * \param dir Histogram output directory
   * \param increaseArea Extend histograms beyond the nominal sensor edge
   * \param maskedPixelRange Track mask around masked pixels, 0 to disable
   * \param inPixelPeriod Folding period in number of pixels
   * \param inPixelMinBins Minimum number of bins along the smaller direction
   */
  BasicEfficiency(const Mechanics::Sensor& sensor,
                  TDirectory* dir,
                  int increaseArea = 2,
                  int maskedPixelRange = 1,
                  int inPixelPeriod = 2,
                  int inPixelMinBins = 32);

  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();

private:
  const Mechanics::Sensor& m_sensor;
  Utils::DenseMask m_mask;
  TH2D* m_total;
  TH1D* m_totalCol;
  TH1D* m_totalRow;
  TH2D* m_pass;
  TH1D* m_passCol;
  TH1D* m_passRow;
  TH2D* m_fail;
  TH1D* m_failCol;
  TH1D* m_failRow;
  TH2D* m_eff;
  TH1D* m_effCol;
  TH1D* m_effRow;
  TH1D* m_effDist;
  XYPoint m_inPixAnchor;
  double m_inPixPeriodU;
  double m_inPixPeriodV;
  TH2D* m_inPixTotal;
  TH2D* m_inPixPass;
  TH2D* m_inPixFail;
  TH2D* m_inPixEff;
};

} // namespace Analyzers

#endif // PT_BASICEFFICIENCY_H
