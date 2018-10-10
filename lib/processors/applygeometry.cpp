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
    Vector2 p2l(sensor.pitchCol(), sensor.pitchRow());

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); icluster++) {
      Storage::Cluster& cluster = sensorEvent.getCluster(icluster);

      auto loc = sensor.transformPixelToLocal(cluster.posPixel());
      auto cov = transformCovariance(p2l.asDiagonal(), cluster.covPixel());
      cluster.setLocal(loc, cov);
    }
  }
}
