#include "applygeometry.h"

#include "mechanics/device.h"
#include "processors/tracking.h"
#include "storage/cluster.h"
#include "storage/event.h"
#include "storage/hit.h"

Processors::ApplyGeometry::ApplyGeometry(const Mechanics::Device& device)
    : m_device(device)
{
}

std::string Processors::ApplyGeometry::name() const { return "ApplyGeometry"; }

void Processors::ApplyGeometry::process(Storage::Event& event) const
{
  assert(event.numPlanes() == m_device.getNumSensors() &&
         "Processors: plane / sensor mismatch");

  for (Index iplane = 0; iplane < event.numPlanes(); iplane++) {
    Storage::Plane& plane = *event.getPlane(iplane);
    const Mechanics::Sensor& sensor = *m_device.getSensor(iplane);

    for (unsigned int icluster = 0; icluster < plane.numClusters(); icluster++)
      plane.getCluster(icluster)->transform(sensor);
  }

  // refit tracks to accomodate possible alignment changes
  for (unsigned int itrack = 0; itrack < event.numTracks(); itrack++)
    Processors::fitTrack(*event.getTrack(itrack));
}

void Processors::applyAlignment(Storage::Event* event,
                                const Mechanics::Device* device)
{
  assert(event && device &&
         "Processors: can't apply alignment with null event and/or device");

  ApplyGeometry(*device).process(*event);
}
