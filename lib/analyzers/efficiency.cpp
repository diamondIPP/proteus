/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2017-02-16
 */

#include "efficiency.h"

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/interval.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(Efficiency)

namespace proteus {

Efficiency::Efficiency(TDirectory* dir,
                       const Sensor& sensor,
                       const int maskedPixelRange,
                       const int increaseArea,
                       const int inPixelPeriod,
                       const int inPixelBinsMin,
                       const int efficiencyDistBins)
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

  TDirectory* sub = makeDir(dir, "sensors/" + sensor.name() + "/efficiency");

  // one set of histograms for the whole sensor
  m_sensorHists = Hists(sub, sensor, sensor.colRowArea(), increaseArea,
                        inPixelPeriod, inPixelBinsMin, efficiencyDistBins);
  // one additional set for each region
  for (const auto& region : sensor.regions()) {
    TDirectory* rsub = makeDir(sub, region.name);
    m_regionsHists.emplace_back(rsub, sensor, region.colRow, increaseArea,
                                inPixelPeriod, inPixelBinsMin,
                                efficiencyDistBins);
  }
}

Efficiency::Hists::Hists(TDirectory* dir,
                         const Sensor& sensor,
                         const DigitalArea& roi,
                         const int increaseArea,
                         const int inPixelPeriod,
                         const int inPixelBinsMin,
                         const int efficiencyDistBins)
    : areaPixel(enlarged(roi, increaseArea))
    , roiPixel(roi)
    , edgeBins(increaseArea)
{

  // define in-pixel submatrix where positions will be folded back to
  Vector4 lowerLeft =
      sensor.transformPixelToLocal(roi.min(0) - 0.5, roi.min(1) - 0.5, 0);
  Vector4 upperRight = sensor.transformPixelToLocal(
      roi.min(0) + inPixelPeriod - 0.5, roi.min(1) + inPixelPeriod - 0.5, 0);
  inPixelAreaLocal = Area(Area::AxisInterval(lowerLeft[kU], upperRight[kU]),
                          Area::AxisInterval(lowerLeft[kV], upperRight[kV]));
  // use approximately quadratic bins in local coords for in-pixel histograms
  auto inPixelBinSize =
      std::min(sensor.pitchCol(), sensor.pitchRow()) / inPixelBinsMin;
  int inPixelBinsU = std::round(inPixelAreaLocal.length(0) / inPixelBinSize);
  int inPixelBinsV = std::round(inPixelAreaLocal.length(1) / inPixelBinSize);

  auto axCol = HistAxis::Integer(areaPixel.interval(0), "Hit column");
  auto axRow = HistAxis::Integer(areaPixel.interval(1), "Hit row");
  auto axInPixU = HistAxis{0, inPixelAreaLocal.length(0), inPixelBinsU,
                           "Folded track position u"};
  auto axInPixV = HistAxis{0, inPixelAreaLocal.length(1), inPixelBinsV,
                           "Folded track position v"};
  auto axEff = HistAxis{0, 1, efficiencyDistBins, "Pixel efficiency"};

  total = makeH2(dir, "tracks_total", axCol, axRow);
  pass = makeH2(dir, "tracks_pass", axCol, axRow);
  fail = makeH2(dir, "tracks_fail", axCol, axRow);
  eff = makeH2(dir, "efficiency", axCol, axRow);
  effDist = makeH1(dir, "efficiency_distribution", axEff);
  colTotal = makeH1(dir, "col_tracks_total", axCol);
  colPass = makeH1(dir, "col_tracks_pass", axCol);
  colFail = makeH1(dir, "col_tracks_fail", axCol);
  colEff = makeH1(dir, "col_efficiency", axCol);
  rowTotal = makeH1(dir, "row_tracks_total", axRow);
  rowPass = makeH1(dir, "row_tracks_pass", axRow);
  rowFail = makeH1(dir, "row_tracks_fail", axRow);
  rowEff = makeH1(dir, "row_efficiency", axRow);
  inPixTotal = makeH2(dir, "inpix_tracks_total", axInPixU, axInPixV);
  inPixPass = makeH2(dir, "inpix_tracks_pass", axInPixU, axInPixV);
  inPixFail = makeH2(dir, "inpix_tracks_fail", axInPixU, axInPixV);
  inPixEff = makeH2(dir, "inpix_efficiency", axInPixU, axInPixV);
  clustersPass = makeH2(dir, "clusters_pass", axCol, axRow);
  clustersFail = makeH2(dir, "clusters_fail", axCol, axRow);
}

std::string Efficiency::name() const
{
  return "Efficiency(" + std::to_string(m_sensor.id()) + ')';
}

void Efficiency::execute(const Event& event)
{
  const SensorEvent& sensorEvent = event.getSensorEvent(m_sensor.id());

  for (const auto& state : sensorEvent.localStates()) {
    auto pix = m_sensor.transformLocalToPixel(state.position());
    auto col = pix[kU];
    auto row = pix[kV];
    // find closest digital pixel address
    int icol = std::round(col);
    int irow = std::round(row);

    // ignore tracks that fall within a masked area
    if (m_mask.isMasked(icol, irow))
      continue;

    // fill efficiency for the whole matrix
    m_sensorHists.fill(state, col, row);
    // fill efficiency for each region
    for (Index iregion = 0; iregion < m_regionsHists.size(); ++iregion) {
      Hists& regionHists = m_regionsHists[iregion];
      // insides region-of-interest + extra edges
      if (!regionHists.areaPixel.isInside(icol, irow))
        continue;
      // ignore tracks that are matched to a cluster in a different region
      if ((state.isMatched()) &&
          (sensorEvent.getCluster(state.matchedCluster()).region() != iregion))
        continue;
      regionHists.fill(state, col, row);
    }
  }
  for (Index icluster = 0; icluster < sensorEvent.numClusters(); ++icluster) {
    const Cluster& cluster = sensorEvent.getCluster(icluster);
    m_sensorHists.fill(cluster);
    if (cluster.hasRegion())
      m_regionsHists[cluster.region()].fill(cluster);
  }
}

void Efficiency::Hists::fill(const TrackState& state, Scalar col, Scalar row)
{
  bool isMatched = state.isMatched();

  total->Fill(col, row);
  if (isMatched) {
    pass->Fill(col, row);
  }

  if (roiPixel.interval(1).isInside(row)) {
    colTotal->Fill(col);
    if (isMatched) {
      colPass->Fill(col);
    }
  }
  if (roiPixel.interval(0).isInside(col)) {
    rowTotal->Fill(row);
    if (isMatched) {
      rowPass->Fill(row);
    }
  }
  if (roiPixel.isInside(col, row)) {
    // calculate folded position
    // TODO 2017-02-17 msmk: can this be done w/ std::remainder or std::fmod?
    double foldedU = state.loc0() - inPixelAreaLocal.min(0);
    double foldedV = state.loc1() - inPixelAreaLocal.min(1);
    foldedU -= inPixelAreaLocal.length(0) *
               std::floor(foldedU / inPixelAreaLocal.length(0));
    foldedV -= inPixelAreaLocal.length(1) *
               std::floor(foldedV / inPixelAreaLocal.length(1));

    inPixTotal->Fill(foldedU, foldedV);
    if (isMatched) {
      inPixPass->Fill(foldedU, foldedV);
    }
  }
}

void Efficiency::Hists::fill(const Cluster& cluster)
{
  if (cluster.isMatched()) {
    clustersPass->Fill(cluster.col(), cluster.row());
  } else {
    clustersFail->Fill(cluster.col(), cluster.row());
  }
}

void Efficiency::finalize()
{
  INFO(m_sensor.name(), " efficiency:");
  m_sensorHists.finalize();

  Index iregion = 0;
  for (auto& hists : m_regionsHists) {
    const auto& region = m_sensor.regions().at(iregion);
    INFO(m_sensor.name(), "/", region.name, " efficiency:");
    hists.finalize();
    iregion += 1;
  }
}

void Efficiency::Hists::finalize()
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
  // overview statistics
  auto cluPass = clustersPass->GetEntries();
  auto cluFail = clustersFail->GetEntries();
  auto cluTotal = cluPass + cluFail;
  auto trkPass = pass->GetEntries();
  auto trkFail = fail->GetEntries();
  auto trkTotal = total->GetEntries();
  auto effMedian = effDist->GetBinCenter(effDist->GetMaximumBin());
  auto effMean = effDist->GetMean();
  INFO("  clusters (pass/fail/total): ", cluPass, "/", cluFail, "/", cluTotal);
  INFO("  tracks (pass/fail/total): ", trkPass, "/", trkFail, "/", trkTotal);
  INFO("  pixel eff (median/mean/min): ", effMedian, "/", effMean, "/", effMin);
}

} // namespace proteus
