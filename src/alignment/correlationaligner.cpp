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
  Mechanics::Alignment geo = m_device.alignment();
  double deltaX = 0;
  double deltaY = 0;

  for (auto id = m_alignIds.begin(); id != m_alignIds.end(); ++id) {
    deltaX -= m_corr->getHistDiffX(*id - 1, *id)->GetMean();
    deltaY -= m_corr->getHistDiffY(*id - 1, *id)->GetMean();

    INFO(m_device.getSensor(*id)->name(), *id, " alignment corrections:");
    INFO("  delta x:  ", deltaX);
    INFO("  delta y:  ", deltaY);

    geo.correctGlobalOffset(*id, deltaX, deltaY, 0);
  }
  return geo;
}
