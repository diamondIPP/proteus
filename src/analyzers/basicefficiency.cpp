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
                                            int increaseArea,
                                            int maskedPixelRange,
                                            int inPixelPeriod,
                                            int inPixelMinBins)
    : m_sensor(sensor)
    , m_inPixAnchor(sensor.sensitiveAreaLocal().min(0),
                    sensor.sensitiveAreaLocal().min(1))
    , m_inPixPeriodU(inPixelPeriod * sensor.pitchCol())
    , m_inPixPeriodV(inPixelPeriod * sensor.pitchRow())
{
  using namespace Utils;

  if (increaseArea < 0)
    FAIL("increaseArea must be positive");
  if (maskedPixelRange < 0)
    FAIL("maskedPixelRange must be positive");
  if (inPixelPeriod < 1)
    FAIL("inPixelPeriod must be 1 or larger");
  if (inPixelMinBins < 0)
    FAIL("inPixelMinBins must be positive");

  if (0 < maskedPixelRange) {
    m_mask = sensor.pixelMask().protruded(maskedPixelRange - 1);
  }

  INFO("in-pixel folded area:");
  INFO("  period: ", inPixelPeriod, " pixels");
  INFO("  u: [", m_inPixAnchor.x(), " + ", m_inPixPeriodU, ")");
  INFO("  v: [", m_inPixAnchor.y(), " + ", m_inPixPeriodV, ")");

  // sensor level efficiency
  // Increase range to be able to see edge effects
  auto area = sensor.sensitiveAreaPixel();
  HistAxis axCol(static_cast<int>(area.min(0)) - increaseArea,
                 static_cast<int>(area.max(0)) + increaseArea, "Hit column");
  HistAxis axRow(static_cast<int>(area.min(1)) - increaseArea,
                 static_cast<int>(area.max(1)) + increaseArea, "Hit row");

  TDirectory* sub = Utils::makeDir(dir, "Efficiency");
  auto name = [&](const char* suffix) { return sensor.name() + '-' + suffix; };
  auto makeColRow = [&](const char* suffix) {
    return makeH2(sub, name(suffix), axCol, axRow);
  };
  auto makeCol = [&](const char* suffix) {
    return makeH1(sub, name(suffix) + "Col", axCol);
  };
  auto makeRow = [&](const char* suffix) {
    return makeH1(sub, name(suffix) + "Row", axCol);
  };

  m_total = makeColRow("TracksTotal");
  m_totalCol = makeCol("TracksTotal");
  m_totalRow = makeRow("TracksTotal");
  m_pass = makeColRow("TracksPass");
  m_passCol = makeCol("TracksPass");
  m_passRow = makeRow("TracksPass");
  m_fail = makeColRow("TracksFail");
  m_failCol = makeCol("TracksFail");
  m_failRow = makeRow("TracksFail");
  m_eff = makeColRow("Efficiency");
  m_effCol = makeCol("Efficiency");
  m_effRow = makeRow("Efficiency");
  m_effDist = Utils::makeH1(sub, sensor.name() + "-EfficiencyDist",
                            {0.0, 1.0, 100, "Pixel efficiency"});

  // in pixel efficiency
  // adjust bin size to have approximately quadratic bins in metric coordinates
  double smallPitch = std::min(sensor.pitchCol(), sensor.pitchRow());
  double binSize = smallPitch / inPixelMinBins;
  double binsU = std::round(m_inPixPeriodU / binSize);
  double binsV = std::round(m_inPixPeriodV / binSize);
  HistAxis axInPixU(0, m_inPixPeriodU, binsU, "Folded track position u");
  HistAxis axInPixV(0, m_inPixPeriodV, binsV, "Folded track position v");
  m_inPixTotal = makeH2(sub, name("InPixTracksTotal"), axInPixU, axInPixV);
  m_inPixPass = makeH2(sub, name("InPixTracksPass"), axInPixU, axInPixV);
  m_inPixFail = makeH2(sub, name("InPixTracksFail"), axInPixU, axInPixV);
  m_inPixEff = makeH2(sub, name("InPixTracksEfficiency"), axInPixU, axInPixV);
}

std::string Analyzers::BasicEfficiency::name() const
{
  return "BasicEfficiency(" + std::to_string(m_sensor.id()) + ')';
}

void Analyzers::BasicEfficiency::analyze(const Storage::Event& event)
{
  const Storage::Plane& sensorEvent = *event.getPlane(m_sensor.id());
  for (Index i = 0; i < sensorEvent.numStates(); ++i) {
    const Storage::TrackState& state = sensorEvent.getState(i);
    auto pos = m_sensor.transformLocalToPixel(state.offset());

    // ignore tracks that fall within a masked area
    if (m_mask.isMasked(pos))
      continue;

    // sensor level efficiency
    m_total->Fill(pos.x(), pos.y());
    m_totalCol->Fill(pos.x());
    m_totalRow->Fill(pos.y());
    if (state.matchedCluster()) {
      m_pass->Fill(pos.x(), pos.y());
      m_passCol->Fill(pos.x());
      m_passRow->Fill(pos.y());
    }
    // in-pixel efficiency
    auto delta = state.offset() - m_inPixAnchor;
    // TODO 2017-02-17 msmk: can this be done w/ std::remainder or std::fmod?
    double foldU =
        delta.x() - m_inPixPeriodU * std::floor(delta.x() / m_inPixPeriodU);
    double foldV =
        delta.y() - m_inPixPeriodV * std::floor(delta.y() / m_inPixPeriodV);
    m_inPixTotal->Fill(foldU, foldV);
    if (state.matchedCluster())
      m_inPixPass->Fill(foldU, foldV);
  }
}

void Analyzers::BasicEfficiency::finalize()
{
  for (auto* h2 : {m_total, m_pass, m_inPixTotal, m_inPixPass})
    h2->Sumw2();
  for (auto* h1 : {m_totalCol, m_totalRow, m_passCol, m_passRow})
    h1->Sumw2();

  // sensor efficiency
  m_fail->Add(m_total, m_pass, 1, -1);
  m_failCol->Add(m_totalCol, m_passCol, 1, -1);
  m_failRow->Add(m_totalRow, m_failRow, 1, -1);
  m_eff->Divide(m_pass, m_total);
  m_effCol->Divide(m_passCol, m_totalCol);
  m_effRow->Divide(m_passRow, m_totalRow);
  // construct the pixel efficiencies distribution
  auto effMin = m_eff->GetMinimum();
  auto effMax = m_eff->GetMaximum();
  m_effDist->SetBins(m_effDist->GetNbinsX(), effMin, effMax);
  for (auto i = m_total->GetNbinsX(); 0 < i; --i) {
    for (auto j = m_total->GetNbinsY(); 0 < j; --j) {
      if (0 < m_total->GetBinContent(i, j))
        m_effDist->Fill(m_eff->GetBinContent(i, j));
    }
  }
  // in-pixel efficiency
  m_inPixFail->Add(m_inPixTotal, m_inPixPass, 1, -1);
  m_inPixEff->Divide(m_inPixPass, m_inPixTotal);

  INFO("efficiency for ", m_sensor.name());
  INFO("  median: ", m_effDist->GetBinCenter(m_effDist->GetMaximumBin()));
  INFO("  mean ", m_effDist->GetMean(), " +-", m_effDist->GetMeanError());
  INFO("  range: ", effMin, " - ", effMax);
}
