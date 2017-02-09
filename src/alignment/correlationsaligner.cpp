#include "correlationsaligner.h"

#include <algorithm>

#include "analyzers/correlation.h"
#include "mechanics/device.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(CorrelationsAligner)

Alignment::CorrelationsAligner::CorrelationsAligner(
    const Mechanics::Device& device,
    const Index fixedId,
    const std::vector<Index>& alignIds,
    TDirectory* dir)
    : m_device(device)
{
  // TODO 2017-02-09 msmk
  // assumes that the fixed sensor is located before the alignment sensors.
  // check which end of the align set is closer and reorder the sensor ids.

  // the fixed sensor must be first entry to get all correlations
  m_sensorIds.reserve(1 + alignIds.size());
  m_sensorIds.push_back(fixedId);
  m_sensorIds.insert(m_sensorIds.end(), alignIds.begin(), alignIds.end());
  std::sort(m_sensorIds.begin() + 1, m_sensorIds.end(),
            Mechanics::CompareSensorIdZ{device});
  // we only need correlations between direct neighbors
  m_corr.reset(new Analyzers::Correlation(device, m_sensorIds, dir, 1));
}

std::string Alignment::CorrelationsAligner::name() const
{
  return "CorrelationsAligner";
}

void Alignment::CorrelationsAligner::analyze(const Storage::Event& event)
{
  m_corr->processEvent(&event);
}

void Alignment::CorrelationsAligner::finalize() { m_corr->postProcessing(); }

Mechanics::Geometry Alignment::CorrelationsAligner::updatedGeometry() const
{
  Mechanics::Geometry geo = m_device.geometry();
  double deltaX = 0, deltaY = 0, deltaXVar = 0, deltaYVar = 0;

  // first sensor is the fixed reference sensor and will not be aligned
  for (size_t i = 0; (i + 1) < m_sensorIds.size(); ++i) {
    Index id0 = m_sensorIds[i];
    Index id1 = m_sensorIds[i + 1];

    const TH1D* hX = m_corr->getHistDiffX(id0, id1);
    deltaX -= hX->GetMean();
    deltaXVar += hX->GetMeanError() * hX->GetMeanError();

    const TH1D* hY = m_corr->getHistDiffY(id0, id1);
    deltaY -= hY->GetMean();
    deltaYVar += hY->GetMeanError() * hY->GetMeanError();

    INFO(m_device.getSensor(id1)->name(), " alignment corrections:");
    INFO("  delta x:  ", deltaX, " +- ", std::sqrt(deltaXVar));
    INFO("  delta y:  ", deltaY, " +- ", std::sqrt(deltaYVar));

    geo.correctGlobalOffset(id1, deltaX, deltaY, 0);
  }
  return geo;
}
