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
    : m_device(device)
    , m_sensorIds(sortedAlongBeam(device.geometry(), alignIds))
{
  // TODO 2017-02-09 msmk
  // assumes that the fixed sensor is located before the alignment sensors.
  // check which end of the align set is closer and reorder the sensor ids.

  // the fixed sensor must be first entry to get the correct correlations
  m_sensorIds.insert(m_sensorIds.begin(), fixedId);
  // we only need correlations between direct neighbors
  m_corr.reset(new Analyzers::Correlations(dir, device, m_sensorIds, 1));
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
  double deltaX = 0, deltaY = 0, deltaXVar = 0, deltaYVar = 0;

  // first sensor is the fixed reference sensor and will not be aligned
  for (size_t i = 0; (i + 1) < m_sensorIds.size(); ++i) {
    Index id0 = m_sensorIds[i];
    Index id1 = m_sensorIds[i + 1];

    const TH1D* hX = m_corr->getHistDiffX(id0, id1);
    deltaX -= hX->GetBinCenter(hX->GetMaximumBin());
    deltaXVar += hX->GetMeanError() * hX->GetMeanError();

    const TH1D* hY = m_corr->getHistDiffY(id0, id1);
    deltaY -= hY->GetBinCenter(hY->GetMaximumBin());
    deltaYVar += hY->GetMeanError() * hY->GetMeanError();

    INFO(m_device.getSensor(id1)->name(), " alignment corrections:");
    INFO("  x0:  ", deltaX, " +- ", std::sqrt(deltaXVar));
    INFO("  y0:  ", deltaY, " +- ", std::sqrt(deltaYVar));

    geo.correctGlobalOffset(id1, deltaX, deltaY, 0);
  }
  return geo;
}
