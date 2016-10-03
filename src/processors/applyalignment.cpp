#include "applyalignment.h"

#include "mechanics/device.h"
#include "processors/trackmaker.h"
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

void Processors::ApplyAlignment::process(uint64_t /* unused */,
                                         Storage::Event& event)
{
  applyAlignment(&event, &m_device);
}

void Processors::ApplyAlignment::finalize() {}

void Processors::applyAlignment(Storage::Event* event,
                                const Mechanics::Device* device)
{
  assert(event && device &&
         "Processors: can't apply alignmet with null event and/or device");
  assert(event->numPlanes() == device->getNumSensors() &&
         "Processors: plane / sensor mismatch");

  for (unsigned int nplane = 0; nplane < event->numPlanes(); nplane++) {
    Storage::Plane* plane = event->getPlane(nplane);
    const Mechanics::Sensor* sensor = device->getSensor(nplane);
    const Transform3D pixelToGlobal = sensor->constructPixelToGlobal();

    for (unsigned int ihit = 0; ihit < plane->numHits(); ihit++)
      plane->getHit(ihit)->transformToGlobal(pixelToGlobal);
    for (unsigned int iclu = 0; iclu < plane->numClusters(); iclu++)
      plane->getCluster(iclu)->transformToGlobal(pixelToGlobal);
  }

  // Apply alignment to tracks
  for (unsigned int ntrack = 0; ntrack < event->numTracks(); ntrack++) {
    Storage::Track* track = event->getTrack(ntrack);
    Processors::TrackMaker::fitTrackToClusters(track);
  }
}
