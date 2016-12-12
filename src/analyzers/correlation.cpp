#include "correlation.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::Correlation::Correlation(const Mechanics::Device& device,
                                    const std::vector<Index>& sensorIds,
                                    TDirectory* dir)
    : SingleAnalyzer(&device, dir, "", "Correlation")
{
  if (sensorIds.size() < 2)
    throw std::runtime_error("Correlation: need at least two sensors");

  TDirectory* corrDir = makeGetDirectory("Correlations");
  for (auto id = sensorIds.begin(); (id + 1) != sensorIds.end(); ++id) {
    addHist(*device.getSensor(*id), *device.getSensor(*(id + 1)), corrDir);
  }
}

Analyzers::Correlation::Correlation(const Mechanics::Device* device,
                                    TDirectory* dir,
                                    const char* suffix)
    : // Base class is initialized here and manages directory / device
    SingleAnalyzer(device, dir, suffix, "Correlation")
{
  assert(device && "Analyzer: can't initialize with null device");

  TDirectory* corrDir = Utils::makeDir(dir, "Correlations");
  for (Index sensorId = 0; (sensorId + 1) < device->numSensors(); ++sensorId)
    addHist(*device->getSensor(sensorId), *device->getSensor(sensorId + 1),
            corrDir);
}

void Analyzers::Correlation::addHist(const Mechanics::Sensor& sensor0,
                                     const Mechanics::Sensor& sensor1,
                                     TDirectory* dir)
{
  using namespace Utils;

  auto makeCorr = [&](int axis) -> TH2D* {
    std::string aname = (axis == 0) ? "X" : "Y";
    std::string name =
        sensor1.name() + "-" + sensor0.name() + "-Correlation" + aname;
    std::string xlabel = sensor0.name() + " cluster " + aname;
    std::string ylabel = sensor1.name() + " cluster " + aname;
    auto range0 = sensor0.projectedEnvelopeXY().interval(axis);
    auto range1 = sensor1.projectedEnvelopeXY().interval(axis);
    auto pitch0 = sensor0.projectedPitchXY()[axis];
    auto pitch1 = sensor1.projectedPitchXY()[axis];
    TH2D* h = new TH2D(name.c_str(), "", range0.length() / pitch0, range0.min(),
                       range0.max(), range1.length() / pitch1, range1.min(),
                       range1.max());
    h->SetXTitle(xlabel.c_str());
    h->SetYTitle(ylabel.c_str());
    h->SetDirectory(dir);
    return h;
  };
  auto makeDiff = [&](int axis) -> TH1D* {
    std::string aname = (axis == 0) ? "X" : "Y";
    std::string name = sensor1.name() + "-" + sensor0.name() + "-Diff" + aname;
    std::string xlabel =
        sensor1.name() + " - " + sensor0.name() + " cluster " + aname;
    auto range0 = sensor0.projectedEnvelopeXY().interval(axis);
    auto range1 = sensor0.projectedEnvelopeXY().interval(axis);
    auto pitch0 = sensor0.projectedPitchXY()[axis];
    auto pitch1 = sensor1.projectedPitchXY()[axis];
    auto pitch = std::min(pitch0, pitch1);
    auto length = range0.length() + range1.length();
    TH1D* h =
        new TH1D(name.c_str(), "", length / pitch, -length / 2, length / 2);
    h->SetXTitle(xlabel.c_str());
    h->SetDirectory(dir);
    return h;
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
  auto idx = std::make_pair(sensorId0, sensorId1);
  return m_hists.at(idx).diffX;
}

TH1D* Analyzers::Correlation::getHistDiffY(Index sensorId0,
                                           Index sensorId1) const
{
  auto idx = std::make_pair(sensorId0, sensorId1);
  return m_hists.at(idx).diffY;
}

TH1D* Analyzers::Correlation::getAlignmentPlotX(Index sensorId) const
{
  return getHistDiffX(sensorId - 1, sensorId);
}

TH1D* Analyzers::Correlation::getAlignmentPlotY(Index sensorId) const
{
  return getHistDiffY(sensorId - 1, sensorId);
}
