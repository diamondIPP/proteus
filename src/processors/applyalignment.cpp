#include "applyalignment.h"

#include "mechanics/device.h"
#include "processors/tracking.h"
#include "storage/cluster.h"
#include "storage/event.h"
#include "storage/hit.h"

Processors::ApplyAlignment::ApplyAlignment(const Mechanics::Device& device)
    : m_device(device)
{
}

std::string Processors::ApplyAlignment::name() const
{
  return "ApplyAlignment";
}

void Processors::ApplyAlignment::process(Storage::Event& event) const
{
  setGeometry(&event, &m_device);
}

void Processors::setGeometry(Storage::Event* event,
                                const Mechanics::Device* device)
{
  assert(event && device &&
         "Processors: can't apply alignment with null event and/or device");
  assert(event->numPlanes() == device->getNumSensors() &&
         "Processors: plane / sensor mismatch");

  for (Index iplane = 0; iplane < event->numPlanes(); iplane++) {
    Storage::Plane* plane = event->getPlane(iplane);
    const Mechanics::Sensor* sensor = device->getSensor(iplane);
    const Transform3D pixelToGlobal = sensor->constructPixelToGlobal();

    for (unsigned int ihit = 0; ihit < plane->numHits(); ihit++)
      plane->getHit(ihit)->transformToGlobal(pixelToGlobal);
    for (unsigned int icluster = 0; icluster < plane->numClusters(); icluster++)
      plane->getCluster(icluster)->transformToGlobal(pixelToGlobal);
  }

  // refit tracks to accomodate possible alignment changes
  for (unsigned int itrack = 0; itrack < event->numTracks(); itrack++)
    Processors::fitTrack(*event->getTrack(itrack));
}
