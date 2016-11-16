#include "correlation.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>

#include "mechanics/device.h"
#include "storage/event.h"

void Analyzers::Correlation::processEvent(const Storage::Event* event)
{
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);

  if (!checkCuts(event))
    return;

  for (auto hist = m_hists.begin(); hist != m_hists.end(); ++hist) {
    const Storage::Plane& plane0 = *event->getPlane(hist->id0);
    const Storage::Plane& plane1 = *event->getPlane(hist->id1);

    for (Index ic0 = 0; ic0 < plane0.numClusters(); ++ic0) {
      const Storage::Cluster* cluster0 = plane0.getCluster(ic0);
      const XYZPoint& xyz0 = cluster0->posGlobal();
      if (!checkCuts(cluster0))
        continue;

      for (Index ic1 = 0; ic1 < plane1.numClusters(); ++ic1) {
        const Storage::Cluster* cluster1 = plane1.getCluster(ic1);
        const XYZPoint& xyz1 = cluster1->posGlobal();
        if (!checkCuts(cluster1))
          continue;

        hist->corrX->Fill(xyz0.x(), xyz1.x());
        hist->corrY->Fill(xyz0.y(), xyz1.y());
        hist->diffX->Fill(xyz1.x() - xyz0.x());
        hist->diffY->Fill(xyz1.y() - xyz0.y());
      }
    }
  }
}

void Analyzers::Correlation::postProcessing() {}

void Analyzers::Correlation::addHist(const Mechanics::Sensor& sensor0,
                                     const Mechanics::Sensor& sensor1,
                                     TDirectory* dir)
{
  int nbins = 100;
  auto makeCorr = [&](int axis) -> TH2D* {
    std::string aname = (axis = 0) ? "X" : "Y";
    std::string name = sensor0.name() + "-" + sensor1.name() + "-Corr" + aname;
    std::string xlabel = sensor0.name() + " cluster " + aname;
    std::string ylabel = sensor1.name() + " cluster " + aname;
    auto range0 = sensor0.sensitiveEnvelopeGlobal().axes[axis];
    auto range1 = sensor1.sensitiveEnvelopeGlobal().axes[axis];
    TH2D* h = new TH2D(name.c_str(), "", nbins, range0.min, range0.max, nbins,
                       range1.min, range1.max);
    h->GetXaxis()->SetTitle(xlabel.c_str());
    h->GetYaxis()->SetTitle(ylabel.c_str());
    h->SetDirectory(dir);
    return h;
  };
  auto makeDiff = [&](int axis) -> TH1D* {
    std::string aname = (axis = 0) ? "X" : "Y";
    std::string name = sensor0.name() + "-" + sensor1.name() + "-Diff" + aname;
    std::string xlabel =
        sensor1.name() + " - " + sensor0.name() + " cluster " + aname;
    auto range0 = sensor0.sensitiveEnvelopeGlobal().axes[axis];
    auto range1 = sensor0.sensitiveEnvelopeGlobal().axes[axis];
    auto length = (range0.length() + range1.length()) / 2;
    TH1D* h = new TH1D(name.c_str(), "", nbins, -length, length);
    h->GetXaxis()->SetTitle(xlabel.c_str());
    h->SetDirectory(dir);
    return h;
  };

  CorrelationHists hist;
  hist.id0 = sensor0.id();
  hist.id1 = sensor1.id();
  hist.corrX = makeCorr(0);
  hist.corrY = makeCorr(1);
  hist.diffX = makeDiff(0);
  hist.diffY = makeDiff(1);
  m_hists.push_back(hist);
}

TH1D* Analyzers::Correlation::getAlignmentPlotX(Index sensorId)
{
  if (sensorId == 0)
    throw std::runtime_error("Correlation: no plot exists for sensor 0");
  // TODO 2016-11-16 msmk: check if correct histogram
  return m_hists.at(sensorId - 1).diffX;
}

TH1D* Analyzers::Correlation::getAlignmentPlotY(Index sensorId)
{
  if (sensorId == 0)
    throw std::runtime_error("Correlation: no plot exists for sensor 0");
  // TODO 2016-11-16 msmk: check if correct histogram
  return m_hists.at(sensorId - 1).diffY;
}

Analyzers::Correlation::Correlation(const Mechanics::Device* device,
                                    TDirectory* dir,
                                    const char* suffix)
    : // Base class is initialized here and manages directory / device
    SingleAnalyzer(device, dir, suffix, "Correlation")
{
  assert(device && "Analyzer: can't initialize with null device");

  TDirectory* corrDir = makeGetDirectory("Correlations");
  for (Index sensorId = 0; (sensorId + 1) < device->numSensors(); ++sensorId)
    addHist(*_device->getSensor(sensorId), *_device->getSensor(sensorId + 1),
            corrDir);
}
