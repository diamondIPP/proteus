/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02-16
 */

#include "basicefficiency.h"

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/interval.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(BasicEfficiency)

Analyzers::BasicEfficiency::BasicEfficiency(const Mechanics::Sensor& sensor,
                                            TDirectory* dir,
                                            int maskedPixelRange,
                                            int increaseArea,
                                            int inPixelPeriod,
                                            int inPixelBinsMin,
                                            int efficiencyDistBins)
    : m_sensor(sensor)
{
  if (maskedPixelRange < 0)
    FAIL("maskedPixelRange must be positive");
  if (increaseArea < 0)
    FAIL("increaseArea must be positive");
  if (inPixelPeriod < 1)
    FAIL("inPixelPeriod must be 1 or larger");
  if (inPixelBinsMin < 1)
    FAIL("inPixelMinBins must be 1 or larger");
  if (efficiencyDistBins < 1)
    FAIL("efficiencyDistBins must be 1 or larger");

  if (0 < maskedPixelRange) {
    m_mask = sensor.pixelMask().protruded(maskedPixelRange - 1);
  }

  TDirectory* sub = Utils::makeDir(dir, "Efficiency");

  // one set of histograms for the whole sensor
  m_sensorHists =
      Hists(sensor.name(), sensor, sensor.sensitiveAreaPixel(), increaseArea,
            inPixelPeriod, inPixelBinsMin, efficiencyDistBins, sub);
  // one additional set for each region
  for (const auto& region : sensor.regions()) {
    m_regionsHists.emplace_back(sensor.name() + '-' + region.name, sensor,
                                region.areaPixel, increaseArea, inPixelPeriod,
                                inPixelBinsMin, efficiencyDistBins, sub);
  }
}

Analyzers::BasicEfficiency::Hists::Hists(const std::string& prefix,
                                         const Mechanics::Sensor& sensor,
                                         Area roi,
                                         int increaseArea,
                                         int inPixelPeriod,
                                         int inPixelBinsMin,
                                         int efficiencyDistBins,
                                         TDirectory* dir)
    : areaPixel(enlarged(roi, increaseArea))
    , roiPixel(roi)
    , edgeBins(increaseArea)
{
  using namespace Utils;

  // define in-pixel submatrix where positions will be folded back to
  XYPoint anchorPix(roi.min(0), roi.min(1));
  XYPoint anchorLoc = sensor.transformPixelToLocal(anchorPix);
  double uMin = anchorLoc.x();
  double uMax = anchorLoc.x() + inPixelPeriod * sensor.pitchCol();
  double vMin = anchorLoc.y();
  double vMax = anchorLoc.y() + inPixelPeriod * sensor.pitchRow();
  inPixelAreaLocal =
      Area(Area::AxisInterval(uMin, uMax), Area::AxisInterval(vMin, vMax));
  // use approximately quadratic bins in local coords for in-pixel histograms
  double inPixelBinSize =
      std::min(sensor.pitchCol(), sensor.pitchRow()) / inPixelBinsMin;
  int inPixelBinsU = std::round(inPixelAreaLocal.length(0) / inPixelBinSize);
  int inPixelBinsV = std::round(inPixelAreaLocal.length(1) / inPixelBinSize);

  HistAxis axCol(areaPixel.interval(0), areaPixel.length(0), "Hit column");
  HistAxis axRow(areaPixel.interval(1), areaPixel.length(1), "Hit row");
  HistAxis axInPixU(0, inPixelAreaLocal.length(0), inPixelBinsU,
                    "Folded track position u");
  HistAxis axInPixV(0, inPixelAreaLocal.length(1), inPixelBinsV,
                    "Folded track position v");
  HistAxis axEff(0, 1, efficiencyDistBins, "Pixel efficiency");

  auto name = [&](const std::string& suffix) { return prefix + '-' + suffix; };
  total = makeH2(dir, name("TracksTotal"), axCol, axRow);
  pass = makeH2(dir, name("TracksPass"), axCol, axRow);
  fail = makeH2(dir, name("TracksFail"), axCol, axRow);
  eff = makeH2(dir, name("Efficiency"), axCol, axRow);
  effDist = makeH1(dir, name("EfficiencyDist"), axEff);
  colTotal = makeH1(dir, name("ColTracksTotal"), axCol);
  colPass = makeH1(dir, name("ColTracksPass"), axCol);
  colFail = makeH1(dir, name("ColTracksFail"), axCol);
  colEff = makeH1(dir, name("ColEfficiency"), axCol);
  rowTotal = makeH1(dir, name("RowTracksTotal"), axRow);
  rowPass = makeH1(dir, name("RowTracksPass"), axRow);
  rowFail = makeH1(dir, name("RowTracksFail"), axRow);
  rowEff = makeH1(dir, name("RowEfficiency"), axRow);
  inPixTotal = makeH2(dir, name("InPixTracksTotal"), axInPixU, axInPixV);
  inPixPass = makeH2(dir, name("InPixTracksPass"), axInPixU, axInPixV);
  inPixFail = makeH2(dir, name("InPixTracksFail"), axInPixU, axInPixV);
  inPixEff = makeH2(dir, name("InPixEfficiency"), axInPixU, axInPixV);
}

std::string Analyzers::BasicEfficiency::name() const
{
  return "BasicEfficiency(" + std::to_string(m_sensor.id()) + ')';
}

void Analyzers::BasicEfficiency::analyze(const Storage::Event& event)
{
  const Storage::Plane& sensorEvent = *event.getPlane(m_sensor.id());
  for (Index istate = 0; istate < sensorEvent.numStates(); ++istate) {
    const Storage::TrackState& state = sensorEvent.getState(istate);
    auto posPixel = m_sensor.transformLocalToPixel(state.offset());

    // ignore tracks that fall within a masked area
    if (m_mask.isMasked(posPixel))
      continue;

    // fill efficiency for the whole matrix
    m_sensorHists.fill(state, posPixel);
    // fill efficiency for each region
    for (Index iregion = 0; iregion < m_regionsHists.size(); ++iregion) {
      Hists& regionHists = m_regionsHists[iregion];
      // insides region-of-interest + extra edges
      if (!regionHists.areaPixel.isInside(posPixel.x(), posPixel.y()))
        continue;
      // ignore tracks that are matched to a cluster in a different region
      if (state.matchedCluster() &&
          (state.matchedCluster()->region() != iregion))
        continue;
      regionHists.fill(state, posPixel);
    }
  }
}

void Analyzers::BasicEfficiency::Hists::fill(const Storage::TrackState& state,
                                             const XYPoint& posPixel)
{
  bool isMatched = state.matchedCluster();

  total->Fill(posPixel.x(), posPixel.y());
  if (isMatched)
    pass->Fill(posPixel.x(), posPixel.y());

  if (roiPixel.interval(1).isInside(posPixel.y())) {
    colTotal->Fill(posPixel.x());
    if (isMatched)
      colPass->Fill(posPixel.x());
  }
  if (roiPixel.interval(0).isInside(posPixel.x())) {
    rowTotal->Fill(posPixel.y());
    if (isMatched)
      rowPass->Fill(posPixel.y());
  }
  if (roiPixel.isInside(posPixel.x(), posPixel.y())) {
    // calculate folded position
    // TODO 2017-02-17 msmk: can this be done w/ std::remainder or std::fmod?
    double foldedU = state.offset().x() - inPixelAreaLocal.min(0);
    double foldedV = state.offset().y() - inPixelAreaLocal.min(1);
    foldedU -= inPixelAreaLocal.length(0) *
               std::floor(foldedU / inPixelAreaLocal.length(0));
    foldedV -= inPixelAreaLocal.length(1) *
               std::floor(foldedV / inPixelAreaLocal.length(1));

    inPixTotal->Fill(foldedU, foldedV);
    if (isMatched)
      inPixPass->Fill(foldedU, foldedV);
  }
}

void Analyzers::BasicEfficiency::finalize()
{
  INFO("efficiency for ", m_sensor.name());
  m_sensorHists.finalize();

  Index iregion = 0;
  for (auto& hists : m_regionsHists) {
    const auto& region = m_sensor.regions().at(iregion);
    INFO("efficiency for ", m_sensor.name(), "/", region.name);
    hists.finalize();
    iregion += 1;
  }
}

void Analyzers::BasicEfficiency::Hists::finalize()
{
  // we just need the plain number differences w/o sumw2
  fail->Add(total, pass, 1, -1);
  colFail->Add(colTotal, colPass, 1, -1);
  rowFail->Add(rowTotal, rowFail, 1, -1);
  inPixFail->Add(inPixTotal, inPixPass, 1, -1);
  // ensure errors are available
  for (TH2D* h : {total, pass, fail, inPixTotal, inPixPass, inPixFail})
    h->Sumw2();
  for (TH1D* h : {colTotal, colPass, colFail, rowTotal, rowPass, rowFail})
    h->Sumw2();
  // Use simple division here w/o full TEfficiency for simplicity.
  eff->Divide(pass, total);
  colEff->Divide(colPass, colTotal);
  rowEff->Divide(rowPass, rowTotal);
  inPixEff->Divide(inPixPass, inPixTotal);
  // construct the pixel efficiencies distribution
  // get minimum efficiency inside the input roi
  double effMin = DBL_MAX;
  for (int i = (1 + edgeBins); (i + edgeBins) <= total->GetNbinsX(); ++i) {
    for (int j = (1 + edgeBins); (j + edgeBins) <= total->GetNbinsY(); ++j) {
      // w/o input tracks we get no efficiency estimate
      if (0 < total->GetBinContent(i, j)) {
        effMin = std::min(effMin, eff->GetBinContent(i, j));
      }
    }
  }
  // make sure 1.0 is still included in the upper bin
  effDist->SetBins(effDist->GetNbinsX(), effMin, std::nextafter(1.0, 2.0));
  for (int i = 1; i <= total->GetNbinsX(); ++i) {
    for (int j = 1; j <= total->GetNbinsY(); ++j) {
      // only add pixels for which we have measurements
      if (0 < total->GetBinContent(i, j)) {
        effDist->Fill(eff->GetBinContent(i, j));
      }
    }
  }

  auto nTotal = total->GetEntries();
  auto nPass = pass->GetEntries();
  auto effMedian = effDist->GetBinCenter(effDist->GetMaximumBin());
  auto effMean = effDist->GetMean();
  INFO("  tracks (total/pass): ", nTotal, "/", nPass);
  INFO("  eff (median/mean/min): ", effMedian, "/", effMean, "/", effMin);
}
