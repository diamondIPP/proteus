#include "applygeometry.h"

#include "mechanics/device.h"
#include "storage/event.h"

Processors::ApplyGeometry::ApplyGeometry(const Mechanics::Device& device)
    : m_device(device)
{
}

std::string Processors::ApplyGeometry::name() const { return "ApplyGeometry"; }

void Processors::ApplyGeometry::execute(Storage::Event& event) const
{
  assert(event.numSensorEvents() == m_device.numSensors() &&
         "Processors: plane / sensor mismatch");

  for (Index iplane = 0; iplane < event.numSensorEvents(); iplane++) {
    Storage::SensorEvent& sensorEvent = event.getSensorEvent(iplane);
    const Mechanics::Sensor& sensor = m_device.getSensor(iplane);

    // jacobian from pixel to local coordinates
    Matrix2 p2l;
    p2l(0, 0) = sensor.pitchCol();
    p2l(0, 1) = 0;
    p2l(1, 0) = 0;
    p2l(1, 1) = sensor.pitchRow();

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); icluster++) {
      Storage::Cluster& cluster = sensorEvent.getCluster(icluster);

      auto loc = sensor.transformPixelToLocal(cluster.posPixel());
      auto cov = Similarity(p2l, cluster.covPixel());

      cluster.setLocal(loc, cov);
    }
  }
}

void Processors::applyAlignment(Storage::Event* event,
                                const Mechanics::Device* device)
{
  assert(event && device &&
         "Processors: can't apply alignment with null event and/or device");

  ApplyGeometry(*device).execute(*event);
}
