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
                                            int edgeExtension)
    : m_sensor(sensor)
{
  if (edgeExtension < 0)
    FAIL("edgeExtension must be 0 or larger");

  // Increase range to be able to see edge effects
  auto area = sensor.sensitiveAreaPixel();
  Utils::HistAxis axCol(static_cast<int>(area.min(0)) - edgeExtension,
                        static_cast<int>(area.max(0)) + edgeExtension,
                        "Hit column");
  Utils::HistAxis axRow(static_cast<int>(area.min(1)) - edgeExtension,
                        static_cast<int>(area.max(1)) + edgeExtension,
                        "Hit row");

  TDirectory* sub = Utils::makeDir(dir, "Efficiency");
  auto makeColRow = [&](const char* name) {
    return Utils::makeH2(sub, sensor.name() + "-" + name, axCol, axRow);
  };
  auto makeCol = [&](const char* name) {
    return Utils::makeH1(sub, sensor.name() + "-" + name + "Col", axCol);
  };
  auto makeRow = [&](const char* name) {
    return Utils::makeH1(sub, sensor.name() + "-" + name + "Row", axCol);
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

    m_total->Fill(pos.x(), pos.y());
    m_totalCol->Fill(pos.x());
    m_totalRow->Fill(pos.y());
    if (state.matchedCluster()) {
      m_pass->Fill(pos.x(), pos.y());
      m_passCol->Fill(pos.x());
      m_passRow->Fill(pos.y());
    }
  }
}

void Analyzers::BasicEfficiency::finalize()
{
  for (auto* h2 : {m_total, m_pass, m_fail, m_eff})
    h2->Sumw2();
  for (auto* h1 : {m_totalCol, m_totalRow, m_passCol, m_passRow})
    h1->Sumw2();

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

  INFO("efficiency for ", m_sensor.name());
  INFO("  median: ", m_effDist->GetBinCenter(m_effDist->GetMaximumBin()));
  INFO("  mean ", m_effDist->GetMean(), " +-", m_effDist->GetMeanError());
  INFO("  range: ", effMin, " - ", effMax);
}
