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

Analyzers::BasicEfficiency::RegionHists::RegionHists(const std::string& prefix,
                                                     Area fullPixel,
                                                     Area foldLocal,
                                                     int foldBinSize,
                                                     TDirectory* dir)
    : areaFull(fullPixel), areaFold(foldLocal)
{
  using namespace Utils;

  // adjust bin size to have approximately quadratic bins in metric coordinates
  int foldBinsU = std::round(foldLocal.length(0) / foldBinSize);
  int foldBinsV = std::round(foldLocal.length(1) / foldBinSize);

  HistAxis axCol(fullPixel.interval(0), fullPixel.length(0), "Hit column");
  HistAxis axRow(fullPixel.interval(1), fullPixel.length(1), "Hit row");
  HistAxis axInPixU(0, foldLocal.length(0), foldBinsU,
                    "Folded track position u");
  HistAxis axInPixV(0, foldLocal.length(1), foldBinsV,
                    "Folded track position v");
  HistAxis axEff(0, 1, 100, "Pixel efficiency");

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
  inPixEff = makeH2(dir, name("InPixTracksEfficiency"), axInPixU, axInPixV);
}

Analyzers::BasicEfficiency::BasicEfficiency(const Mechanics::Sensor& sensor,
                                            TDirectory* dir,
                                            int increaseArea,
                                            int maskedPixelRange,
                                            int inPixelPeriod,
                                            int inPixelBinsMin)
    : m_sensor(sensor)
{
  if (increaseArea < 0)
    FAIL("increaseArea must be positive");
  if (maskedPixelRange < 0)
    FAIL("maskedPixelRange must be positive");
  if (inPixelPeriod < 1)
    FAIL("inPixelPeriod must be 1 or larger");
  if (inPixelBinsMin < 0)
    FAIL("inPixelMinBins must be positive");

  if (0 < maskedPixelRange) {
    m_mask = sensor.pixelMask().protruded(maskedPixelRange - 1);
  }

  // construct folding area for in-pixel efficiency
  auto makeFoldArea = [&](const Area& fullArea) {
    auto uMin = fullArea.min(0);
    auto uMax = uMin + inPixelPeriod * sensor.pitchCol();
    auto vMin = fullArea.min(1);
    auto vMax = vMin + inPixelPeriod * sensor.pitchRow();
    return Area(Area::AxisInterval(uMin, uMax), Area::AxisInterval(vMin, vMax));
  };
  // approximate bin size in folded histograms
  double smallPitch = std::min(sensor.pitchCol(), sensor.pitchRow());
  double foldBinSize = smallPitch / inPixelBinsMin;

  TDirectory* sub = Utils::makeDir(dir, "Efficiency");

  m_whole = RegionHists(
      sensor.name(), enlarged(sensor.sensitiveAreaPixel(), increaseArea),
      makeFoldArea(sensor.sensitiveAreaLocal()), foldBinSize, sub);
}

std::string Analyzers::BasicEfficiency::name() const
{
  return "BasicEfficiency(" + std::to_string(m_sensor.id()) + ')';
}

void Analyzers::BasicEfficiency::RegionHists::fill(
    const Storage::TrackState& state, const XYPoint& posPixel)
{
  // calculate folded position
  // TODO 2017-02-17 msmk: can this be done w/ std::remainder or std::fmod?
  double foldedU = state.offset().x() - areaFold.min(0);
  double foldedV = state.offset().y() - areaFold.min(1);
  foldedU -= areaFold.length(0) * std::floor(foldedU / areaFold.length(0));
  foldedV -= areaFold.length(1) * std::floor(foldedV / areaFold.length(1));

  total->Fill(posPixel.x(), posPixel.y());
  colTotal->Fill(posPixel.x());
  rowTotal->Fill(posPixel.y());
  inPixTotal->Fill(foldedU, foldedV);
  if (state.matchedCluster()) {
    pass->Fill(posPixel.x(), posPixel.y());
    colPass->Fill(posPixel.x());
    rowPass->Fill(posPixel.y());
    inPixPass->Fill(foldedU, foldedV);
  }
}

void Analyzers::BasicEfficiency::analyze(const Storage::Event& event)
{
  const Storage::Plane& sensorEvent = *event.getPlane(m_sensor.id());
  for (Index i = 0; i < sensorEvent.numStates(); ++i) {
    const Storage::TrackState& state = sensorEvent.getState(i);
    auto posPixel = m_sensor.transformLocalToPixel(state.offset());

    // ignore tracks that fall within a masked area
    if (m_mask.isMasked(posPixel))
      continue;

    m_whole.fill(state, posPixel);
  }
}

void Analyzers::BasicEfficiency::RegionHists::finalize()
{
  for (auto* h2 : {total, pass, inPixTotal, inPixPass})
    h2->Sumw2();
  for (auto* h1 : {colTotal, rowTotal, colPass, rowPass})
    h1->Sumw2();

  fail->Add(total, pass, 1, -1);
  colFail->Add(colTotal, colPass, 1, -1);
  rowFail->Add(rowTotal, rowFail, 1, -1);
  inPixFail->Add(inPixTotal, inPixPass, 1, -1);
  // Use simple division here w/o full TEfficiency power for simplicity.
  eff->Divide(pass, total);
  colEff->Divide(colPass, colTotal);
  rowEff->Divide(rowPass, rowTotal);
  inPixEff->Divide(inPixPass, inPixTotal);
  // construct the pixel efficiencies distribution
  effDist->SetBins(effDist->GetNbinsX(), eff->GetMinimum(),
                   std::nextafter(eff->GetMaximum(), eff->GetMaximum() + 1  ));
  for (int i = 1; i <= total->GetNbinsX(); ++i) {
    for (int j = 1; j <= total->GetNbinsY(); ++j) {
      if (0 < total->GetBinContent(i, j)) {
        effDist->Fill(eff->GetBinContent(i, j));
      }
    }
  }

  INFO("  median: ", effDist->GetBinCenter(effDist->GetMaximumBin()));
  INFO("  mean ", effDist->GetMean(), " +-", effDist->GetMeanError());
  INFO("  range: ", eff->GetMinimum(), " - ", eff->GetMaximum());
}

void Analyzers::BasicEfficiency::finalize()
{
  INFO("efficiency for ", m_sensor.name());
  m_whole.finalize();
}
