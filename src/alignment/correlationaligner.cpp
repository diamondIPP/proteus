#include "correlationaligner.h"

#include "analyzers/correlation.h"
#include "mechanics/device.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(CorrelationAligner)

Alignment::CorrelationAligner::CorrelationAligner(
    const Mechanics::Device& device,
    const std::vector<Index>& alignIds,
    std::shared_ptr<const Analyzers::Correlation> corr)
    : m_device(device), m_alignIds(alignIds), m_corr(std::move(corr))
{
}

std::string Alignment::CorrelationAligner::name() const
{
  return "CorrelationAligner";
}

void Alignment::CorrelationAligner::analyze(const Storage::Event& event)
{
  // nothing to do
}

void Alignment::CorrelationAligner::finalize()
{
  // nothing to do
}

Mechanics::Alignment Alignment::CorrelationAligner::updatedGeometry() const
{
  Mechanics::Alignment geo = m_device.geometry();
  double deltaX = 0, deltaXVar = 0;
  double deltaY = 0, deltaYVar = 0;

  for (auto id = m_alignIds.begin(); id != m_alignIds.end(); ++id) {
    const TH1D* hX = m_corr->getHistDiffX(*id - 1, *id);
    deltaX -= hX->GetMean();
    deltaXVar += hX->GetMeanError() * hX->GetMeanError();

    const TH1D* hY = m_corr->getHistDiffY(*id - 1, *id);
    deltaY -= hY->GetMean();
    deltaYVar += hY->GetMeanError() * hY->GetMeanError();

    INFO(m_device.getSensor(*id)->name(), *id, " alignment corrections:");
    INFO("  delta x:  ", deltaX, " +- ", std::sqrt(deltaXVar));
    INFO("  delta y:  ", deltaY, " +- ", std::sqrt(deltaYVar));

    geo.correctGlobalOffset(*id, deltaX, deltaY, 0);
  }
  return geo;
}
