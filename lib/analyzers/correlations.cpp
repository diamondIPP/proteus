#include "correlations.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>

#include <TDirectory.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/logger.h"
#include "utils/root.h"

PT_SETUP_LOCAL_LOGGER(Correlations)

namespace proteus {

Correlations::Correlations(TDirectory* dir,
                           const Device& dev,
                           const std::vector<Index>& sensorIds,
                           const int neighbors)
    : m_geo(dev.geometry())
{
  if (sensorIds.size() < 2)
    FAIL("need at least two sensors but ", sensorIds.size(), " given");
  if (neighbors < 1)
    FAIL("need at least one neighbor but ", neighbors, " given");

  TDirectory* sub = makeDir(dir, "correlations");

  // correlation between selected number of neighboring sensors
  const size_t n = sensorIds.size();
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = 1 + i; j < std::min(n, 1 + i + neighbors); ++j) {
      const Sensor& sensor0 = dev.getSensor(sensorIds[i]);
      const Sensor& sensor1 = dev.getSensor(sensorIds[j]);
      addHist(sensor0, sensor1, sub);
    }
  }
}

Correlations::Correlations(TDirectory* dir,
                           const Device& device,
                           const int neighbors)
    : Correlations(dir,
                   device,
                   sortedAlongBeam(device.geometry(), device.sensorIds()),
                   neighbors)
{
}

std::string Correlations::name() const { return "Correlations"; }

void Correlations::addHist(const Sensor& sensor0,
                           const Sensor& sensor1,
                           TDirectory* dir)
{

  TDirectory* sub = makeDir(dir, sensor0.name() + "-" + sensor1.name());

  auto makeCorr = [&](int dim, std::string name, std::string label) {
    auto range0 = sensor0.projectedBoundingBox().interval(dim);
    auto range1 = sensor1.projectedBoundingBox().interval(dim);
    int bins0 = range0.length() / sensor0.projectedPitch()[dim];
    int bins1 = range1.length() / sensor1.projectedPitch()[dim];
    auto axis0 = HistAxis{range0, bins0, sensor0.name() + " cluster " + label};
    auto axis1 = HistAxis{range1, bins1, sensor1.name() + " cluster " + label};
    return makeH2(sub, "correlation_" + name, axis0, axis1);
  };
  auto makeDiff = [&](int dim, std::string name, std::string label) {
    auto range0 = sensor0.projectedBoundingBox().interval(dim);
    auto range1 = sensor1.projectedBoundingBox().interval(dim);
    auto pitch0 = sensor0.projectedPitch()[dim];
    auto pitch1 = sensor1.projectedPitch()[dim];
    auto axis = HistAxis::Difference(range0, pitch0, range1, pitch1,
                                     sensor1.name() + " - " + sensor0.name() +
                                         " cluster " + label);
    return makeH1(sub, "difference_" + name, axis);
  };

  Hists hist;
  hist.corrX = makeCorr(kX, "x", "position x");
  hist.corrY = makeCorr(kY, "y", "position y");
  hist.corrT = makeCorr(kT, "time", "global time");
  hist.diffX = makeDiff(kX, "x", "position x");
  hist.diffY = makeDiff(kY, "y", "position y");
  hist.diffT = makeDiff(kT, "time", "global time");
  m_hists[std::make_pair(sensor0.id(), sensor1.id())] = hist;
}

void Correlations::execute(const Event& event)
{
  for (auto& entry : m_hists) {
    Index id0 = entry.first.first;
    Index id1 = entry.first.second;
    Hists& hist = entry.second;
    const Plane& plane0 = m_geo.getPlane(id0);
    const Plane& plane1 = m_geo.getPlane(id1);
    const SensorEvent& sensor0 = event.getSensorEvent(id0);
    const SensorEvent& sensor1 = event.getSensorEvent(id1);

    for (Index c0 = 0; c0 < sensor0.numClusters(); ++c0) {
      Vector4 global0 = plane0.toGlobal(sensor0.getCluster(c0).position());

      for (Index c1 = 0; c1 < sensor1.numClusters(); ++c1) {
        Vector4 global1 = plane1.toGlobal(sensor1.getCluster(c1).position());

        hist.corrX->Fill(global0[kX], global1[kX]);
        hist.corrY->Fill(global0[kY], global1[kY]);
        hist.corrT->Fill(global0[kT], global1[kT]);
        hist.diffX->Fill(global1[kX] - global0[kX]);
        hist.diffY->Fill(global1[kY] - global0[kY]);
        hist.diffT->Fill(global1[kT] - global0[kT]);
      }
    }
  }
}

const TH1D* Correlations::getHistDiffX(Index sensorId0, Index sensorId1) const
{
  return m_hists.at(std::make_pair(sensorId0, sensorId1)).diffX;
}

const TH1D* Correlations::getHistDiffY(Index sensorId0, Index sensorId1) const
{
  return m_hists.at(std::make_pair(sensorId0, sensorId1)).diffY;
}

} // namespace proteus
