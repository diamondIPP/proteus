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

Analyzers::Correlations::Correlations(TDirectory* dir,
                                      const Mechanics::Device& dev,
                                      const std::vector<Index>& sensorIds,
                                      const int neighbors)
{
  if (sensorIds.size() < 2)
    FAIL("need at least two sensors but ", sensorIds.size(), " given");
  if (neighbors < 1)
    FAIL("need at least one neighbor but ", neighbors, " given");

  TDirectory* sub = Utils::makeDir(dir, "correlations");

  // correlation between selected number of neighboring sensors
  const size_t n = sensorIds.size();
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = 1 + i; j < std::min(n, 1 + i + neighbors); ++j) {
      const Mechanics::Sensor& sensor0 = *dev.getSensor(sensorIds[i]);
      const Mechanics::Sensor& sensor1 = *dev.getSensor(sensorIds[j]);
      addHist(sensor0, sensor1, sub);
    }
  }
}

Analyzers::Correlations::Correlations(TDirectory* dir,
                                      const Mechanics::Device& device,
                                      const int neighbors)
    : Correlations(dir, device, device.sensorIds(), neighbors)
{
}

std::string Analyzers::Correlations::name() const { return "Correlations"; }

void Analyzers::Correlations::addHist(const Mechanics::Sensor& sensor0,
                                      const Mechanics::Sensor& sensor1,
                                      TDirectory* dir)
{
  using namespace Utils;

  TDirectory* sub = makeDir(dir, sensor0.name() + "-" + sensor1.name());

  auto makeCorr = [&](int axis) {
    std::string axisName = (axis == 0) ? "x" : "y";
    std::string histName = "correlation_" + axisName;
    std::string xlabel = sensor0.name() + " cluster " + axisName;
    std::string ylabel = sensor1.name() + " cluster " + axisName;
    auto range0 = sensor0.projectedEnvelopeXY().interval(axis);
    auto range1 = sensor1.projectedEnvelopeXY().interval(axis);
    int bins0 = range0.length() / sensor0.projectedPitchXY()[axis];
    int bins1 = range1.length() / sensor1.projectedPitchXY()[axis];
    return makeH2(sub, histName, {range0, bins0, xlabel},
                  {range1, bins1, ylabel});
  };
  auto makeDiff = [&](int axis) {
    std::string axisName = (axis == 0) ? "x" : "y";
    std::string histName = "diff_" + axisName;
    std::string xlabel =
        sensor0.name() + " - " + sensor1.name() + " cluster " + axisName;
    double length0 = sensor0.projectedEnvelopeXY().length(axis);
    double length1 = sensor1.projectedEnvelopeXY().length(axis);
    double maxDist = (length0 + length1) / 4;
    double pitch0 = sensor0.projectedPitchXY()[axis];
    double pitch1 = sensor1.projectedPitchXY()[axis];
    int bins = 2 * maxDist / std::min(pitch0, pitch1);
    return makeH1(sub, histName, {-maxDist, maxDist, bins, xlabel});
  };

  Hists hist;
  hist.corrX = makeCorr(0);
  hist.corrY = makeCorr(1);
  hist.diffX = makeDiff(0);
  hist.diffY = makeDiff(1);
  m_hists[std::make_pair(sensor0.id(), sensor1.id())] = hist;
}

void Analyzers::Correlations::analyze(const Storage::Event& event)
{
  for (auto& entry : m_hists) {
    Index id0 = entry.first.first;
    Index id1 = entry.first.second;
    Hists& hist = entry.second;
    const Storage::SensorEvent& sensor0 = event.getSensorEvent(id0);
    const Storage::SensorEvent& sensor1 = event.getSensorEvent(id1);

    for (Index c0 = 0; c0 < sensor0.numClusters(); ++c0) {
      const Storage::Cluster* cluster0 = sensor0.getCluster(c0);
      const XYZPoint& xyz0 = cluster0->posGlobal();

      for (Index c1 = 0; c1 < sensor1.numClusters(); ++c1) {
        const Storage::Cluster* cluster1 = sensor1.getCluster(c1);
        const XYZPoint& xyz1 = cluster1->posGlobal();

        hist.corrX->Fill(xyz0.x(), xyz1.x());
        hist.corrY->Fill(xyz0.y(), xyz1.y());
        hist.diffX->Fill(xyz1.x() - xyz0.x());
        hist.diffY->Fill(xyz1.y() - xyz0.y());
      }
    }
  }
}

void Analyzers::Correlations::finalize() {}

const TH1D* Analyzers::Correlations::getHistDiffX(Index sensorId0,
                                                  Index sensorId1) const
{
  return m_hists.at(std::make_pair(sensorId0, sensorId1)).diffX;
}

const TH1D* Analyzers::Correlations::getHistDiffY(Index sensorId0,
                                                  Index sensorId1) const
{
  return m_hists.at(std::make_pair(sensorId0, sensorId1)).diffY;
}
