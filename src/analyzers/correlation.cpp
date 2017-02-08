#include "correlation.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::Correlation::Correlation(const Mechanics::Device& dev,
                                    const std::vector<Index>& sensorIds,
                                    TDirectory* dir)
    : SingleAnalyzer(&dev, dir, "", "Correlation")
{
  TDirectory* sub = Utils::makeDir(dir, "Correlations");

  if (sensorIds.size() < 2) {
    throw std::runtime_error("Correlation: need at least two sensors");
  } else if (sensorIds.size() == 2) {
    addHist(*dev.getSensor(sensorIds[0]), *dev.getSensor(sensorIds[1]), sub);
  } else {
    // correlation between two nearest planes
    for (auto id = sensorIds.begin(); (id + 2) != sensorIds.end(); ++id) {
      addHist(*dev.getSensor(*id), *dev.getSensor(*(id + 1)), sub);
      addHist(*dev.getSensor(*id), *dev.getSensor(*(id + 2)), sub);
    }
  }
}

Analyzers::Correlation::Correlation(const Mechanics::Device* device,
                                    TDirectory* dir,
                                    const char* /* unused */)
    : Correlation(*device, sortedByZ(*device, device->sensorIds()), dir)
{
  assert(device && "Analyzer: can't initialize with null device");
}

void Analyzers::Correlation::addHist(const Mechanics::Sensor& sensor0,
                                     const Mechanics::Sensor& sensor1,
                                     TDirectory* dir)
{
  using namespace Utils;

  auto makeCorr = [&](int axis) {
    std::string axisName = (axis == 0) ? "X" : "Y";
    std::string histName =
        sensor1.name() + "-" + sensor0.name() + "-Correlation" + axisName;
    std::string xlabel = sensor0.name() + " cluster " + axisName;
    std::string ylabel = sensor1.name() + " cluster " + axisName;
    auto range0 = sensor0.projectedEnvelopeXY().interval(axis);
    auto range1 = sensor1.projectedEnvelopeXY().interval(axis);
    int bins0 = range0.length() / sensor0.projectedPitchXY()[axis];
    int bins1 = range1.length() / sensor1.projectedPitchXY()[axis];
    return makeH2(dir, histName, {range0, bins0, xlabel},
                  {range1, bins1, ylabel});
  };
  auto makeDiff = [&](int axis) {
    std::string axisName = (axis == 0) ? "X" : "Y";
    std::string histName =
        sensor1.name() + "-" + sensor0.name() + "-Diff" + axisName;
    std::string xlabel =
        sensor1.name() + " - " + sensor0.name() + " cluster " + axisName;
    double length0 = sensor0.projectedEnvelopeXY().length(axis);
    double length1 = sensor1.projectedEnvelopeXY().length(axis);
    double maxDist = (length0 + length1) / 4;
    double pitch0 = sensor0.projectedPitchXY()[axis];
    double pitch1 = sensor1.projectedPitchXY()[axis];
    int bins = 2 * maxDist / std::min(pitch0, pitch1);
    return makeH1(dir, histName, {-maxDist, maxDist, bins, xlabel});
  };

  Hists hist;
  hist.corrX = makeCorr(0);
  hist.corrY = makeCorr(1);
  hist.diffX = makeDiff(0);
  hist.diffY = makeDiff(1);
  m_hists[std::make_pair(sensor0.id(), sensor1.id())] = hist;
}

void Analyzers::Correlation::processEvent(const Storage::Event* event)
{
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);

  if (!checkCuts(event))
    return;

  for (auto it = m_hists.begin(); it != m_hists.end(); ++it) {
    Index id0 = it->first.first;
    Index id1 = it->first.second;
    const Hists& hist = it->second;
    const Storage::Plane& plane0 = *event->getPlane(id0);
    const Storage::Plane& plane1 = *event->getPlane(id1);

    for (Index c0 = 0; c0 < plane0.numClusters(); ++c0) {
      const Storage::Cluster* cluster0 = plane0.getCluster(c0);
      const XYZPoint& xyz0 = cluster0->posGlobal();
      if (!checkCuts(cluster0))
        continue;

      for (Index c1 = 0; c1 < plane1.numClusters(); ++c1) {
        const Storage::Cluster* cluster1 = plane1.getCluster(c1);
        const XYZPoint& xyz1 = cluster1->posGlobal();
        if (!checkCuts(cluster1))
          continue;

        hist.corrX->Fill(xyz0.x(), xyz1.x());
        hist.corrY->Fill(xyz0.y(), xyz1.y());
        hist.diffX->Fill(xyz1.x() - xyz0.x());
        hist.diffY->Fill(xyz1.y() - xyz0.y());
      }
    }
  }
}

void Analyzers::Correlation::postProcessing() {}

TH1D* Analyzers::Correlation::getHistDiffX(Index sensorId0,
                                           Index sensorId1) const
{
  return m_hists.at(std::make_pair(sensorId0, sensorId1)).diffX;
}

TH1D* Analyzers::Correlation::getHistDiffY(Index sensorId0,
                                           Index sensorId1) const
{
  return m_hists.at(std::make_pair(sensorId0, sensorId1)).diffY;
}

TH1D* Analyzers::Correlation::getAlignmentPlotX(Index sensorId) const
{
  return getHistDiffX(sensorId - 1, sensorId);
}

TH1D* Analyzers::Correlation::getAlignmentPlotY(Index sensorId) const
{
  return getHistDiffY(sensorId - 1, sensorId);
}
