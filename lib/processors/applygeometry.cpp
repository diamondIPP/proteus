#include "applygeometry.h"

#include "mechanics/device.h"
#include "storage/event.h"

Processors::ApplyGeometry::ApplyGeometry(const Mechanics::Device& device)
    : m_device(device)
{
}

std::string Processors::ApplyGeometry::name() const { return "ApplyGeometry"; }

void Processors::ApplyGeometry::process(Storage::Event& event) const
{
  assert(event.numPlanes() == m_device.numSensors() &&
         "Processors: plane / sensor mismatch");

  for (Index iplane = 0; iplane < event.numPlanes(); iplane++) {
    Storage::Plane& plane = *event.getPlane(iplane);
    const Mechanics::Sensor& sensor = *m_device.getSensor(iplane);

    for (unsigned int icluster = 0; icluster < plane.numClusters(); icluster++)
      plane.getCluster(icluster)->transform(sensor);
  }
}

void Processors::applyAlignment(Storage::Event* event,
                                const Mechanics::Device* device)
{
  assert(event && device &&
         "Processors: can't apply alignment with null event and/or device");

  ApplyGeometry(*device).process(*event);
}
