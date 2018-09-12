#include "correlationsaligner.h"

#include <algorithm>

#include "analyzers/correlations.h"
#include "mechanics/device.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(CorrelationsAligner)

Alignment::CorrelationsAligner::CorrelationsAligner(
    TDirectory* dir,
    const Mechanics::Device& device,
    const Index fixedId,
    const std::vector<Index>& alignIds)
    : m_device(device), m_fixedId(fixedId)
{
  // All sensors sorted along beam
  std::vector<Index> sortedIds(alignIds);
  sortedIds.push_back(fixedId);
  sortedIds = sortedAlongBeam(device.geometry(), sortedIds);

  // we only need correlations between direct neighbors
  m_corr.reset(new Analyzers::Correlations(dir, device, sortedIds, 1));

  // split sensors into two groups: before and after the fixed sensor
  auto itFixed = std::find(sortedIds.begin(), sortedIds.end(), fixedId);
  // order must always go away from the fixed sensor to either end
  std::reverse_copy(sortedIds.begin(), itFixed, std::back_inserter(m_backwardIds));
  std::copy(++itFixed, sortedIds.end(), std::back_inserter(m_forwardIds));
}

// required to make correlations unique_ptr work
Alignment::CorrelationsAligner::~CorrelationsAligner() {}

std::string Alignment::CorrelationsAligner::name() const
{
  return "CorrelationsAligner";
}

void Alignment::CorrelationsAligner::execute(const Storage::Event& event)
{
  m_corr->execute(event);
}

void Alignment::CorrelationsAligner::finalize() { m_corr->finalize(); }

Mechanics::Geometry Alignment::CorrelationsAligner::updatedGeometry() const
{
  Mechanics::Geometry geo = m_device.geometry();

  // align sensors that are located before the fixed sensor
  Index nextId = m_fixedId;
  double deltaX = 0;
  double deltaXVar = 0;
  double deltaY = 0;
  double deltaYVar = 0;
  // backward ids are ordered opposite the beam direction
  for (Index currId : m_backwardIds) {
    const TH1D* hX = m_corr->getHistDiffX(currId, nextId);
    const TH1D* hY = m_corr->getHistDiffY(currId, nextId);

    // correlation was calculated in reverse order and we get an
    // additional sign for the correction
    deltaX += hX->GetBinCenter(hX->GetMaximumBin());
    deltaXVar += hX->GetMeanError() * hX->GetMeanError();
    deltaY += hY->GetBinCenter(hY->GetMaximumBin());
    deltaYVar += hY->GetMeanError() * hY->GetMeanError();

    INFO(m_device.getSensor(currId)->name(),
         " alignment corrections (before fixed sensor):");
    INFO("  dx:  ", deltaX, " +- ", std::sqrt(deltaXVar));
    INFO("  dy:  ", deltaY, " +- ", std::sqrt(deltaYVar));
    geo.correctGlobalOffset(currId, deltaX, deltaY, 0);

    nextId = currId;
  }

  // align sensors that are located after the fixed sensor
  Index prevId = m_fixedId;
  deltaX = 0;
  deltaY = 0;
  deltaXVar = 0;
  deltaYVar = 0;
  // forward ids are ordered along the beam direction
  for (Index currId : m_forwardIds) {
    const TH1D* hX = m_corr->getHistDiffX(prevId, currId);
    const TH1D* hY = m_corr->getHistDiffY(prevId, currId);

    deltaX -= hX->GetBinCenter(hX->GetMaximumBin());
    deltaXVar += hX->GetMeanError() * hX->GetMeanError();
    deltaY -= hY->GetBinCenter(hY->GetMaximumBin());
    deltaYVar += hY->GetMeanError() * hY->GetMeanError();

    INFO(m_device.getSensor(currId)->name(),
         " alignment corrections (after fixed sensor):");
    INFO("  dx:  ", deltaX, " +- ", std::sqrt(deltaXVar));
    INFO("  dy:  ", deltaY, " +- ", std::sqrt(deltaYVar));

    geo.correctGlobalOffset(currId, deltaX, deltaY, 0);

    prevId = currId;
  }
  return geo;
}
