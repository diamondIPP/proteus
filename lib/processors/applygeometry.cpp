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
    DiagMatrix4 scalePitch = sensor.pitch().asDiagonal();

    for (Index icluster = 0; icluster < sensorEvent.numClusters(); icluster++) {
      Storage::Cluster& cluster = sensorEvent.getCluster(icluster);

      SymMatrix4 cov = SymMatrix4::Zero();
      cov(kU, kU) = cluster.colVar();
      cov(kV, kU) = cov(kU, kV) = cluster.colRowCov();
      cov(kV, kV) = cluster.rowVar();
      cov(kS, kS) = cluster.timestampVar();
      cluster.setLocal(sensor.transformPixelToLocal(
                           cluster.col(), cluster.row(), cluster.timestamp()),
                       transformCovariance(scalePitch, cov));
    }
  }
}
