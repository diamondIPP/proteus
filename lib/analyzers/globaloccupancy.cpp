#include "globaloccupancy.h"

#include <cassert>

#include <TDirectory.h>
#include <TH2D.h>

#include "mechanics/device.h"
#include "storage/event.h"
#include "utils/root.h"

Analyzers::GlobalOccupancy::GlobalOccupancy(TDirectory* dir,
                                            const Mechanics::Device& device)
{
  // estimate bounding box of all sensor projections into the xy-plane
  auto active = Mechanics::Sensor::Volume::Empty();
  for (auto isensor : device.sensorIds()) {
    active = Utils::boundingBox(active,
                                device.getSensor(isensor)->projectedEnvelope());
  }

  // create per-sensor histograms
  for (auto isensor : device.sensorIds()) {
    const auto& sensor = *device.getSensor(isensor);
    auto pitch = sensor.projectedPitch();

    Utils::HistAxis ax(active.min(0), active.max(0),
                       active.length(0) / pitch[0], "Global x position");
    Utils::HistAxis ay(active.min(1), active.max(1),
                       active.length(1) / pitch[1], "Global y position");

    TDirectory* sub = Utils::makeDir(dir, "global/" + sensor.name());
    SensorHists h;
    h.plane = device.geometry().getPlane(isensor);
    h.id = isensor;
    h.clusters_xy = Utils::makeH2(sub, "clusters_xy", ax, ay);
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

    for (Index ic = 0; ic < se.numClusters(); ++ic) {
      Vector3 xyz = hists.plane.toGlobal(se.getCluster(ic).posLocal());
      hists.clusters_xy->Fill(xyz[0], xyz[1]);
    }
  }
}
