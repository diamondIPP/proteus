#include "globaloccupancy.h"

#include <cassert>

#include <TDirectory.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::GlobalOccupancy::GlobalOccupancy(TDirectory* dir,
                                            const Mechanics::Device& device)
    : m_geo(device.geometry())
{
  using namespace Utils;

  auto box = device.boundingBox();
  // create per-sensor histograms
  for (auto isensor : device.sensorIds()) {
    const auto& sensor = device.getSensor(isensor);
    auto pitch = sensor.projectedPitch();

    HistAxis ax(box.interval(kX), box.length(kX) / pitch[kX],
                "Cluster position x");
    HistAxis ay(box.interval(kY), box.length(kY) / pitch[kY],
                "Cluster position y");
    HistAxis at(box.interval(kT), box.length(kT) / pitch[kT],
                "Cluster global time");

    TDirectory* sub = makeDir(dir, "global/" + sensor.name());
    SensorHists h;
    h.id = isensor;
    h.clustersXY = makeH2(sub, "clusters_xy", ax, ay);
    h.clustersT = makeH1(sub, "clusters_time", at);
    m_sensorHists.push_back(std::move(h));
  }
}

std::string Analyzers::GlobalOccupancy::name() const
{
  return "GlobalOccupancy";
}

void Analyzers::GlobalOccupancy::execute(const Storage::Event& event)
{
  for (auto& hists : m_sensorHists) {
    const Storage::SensorEvent& se = event.getSensorEvent(hists.id);
    const Mechanics::Plane& plane = m_geo.getPlane(hists.id);

    for (Index ic = 0; ic < se.numClusters(); ++ic) {
      Vector4 global = plane.toGlobal(se.getCluster(ic).position());
      hists.clustersXY->Fill(global[kX], global[kY]);
      hists.clustersT->Fill(global[kT]);
    }
  }
}
