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
  assert(event->getNumPlanes() == device->getNumSensors() &&
         "Processors: plane / sensor mismatch");

  for (unsigned int nplane = 0; nplane < event->getNumPlanes(); nplane++) {
    Storage::Plane* plane = event->getPlane(nplane);
    const Mechanics::Sensor* sensor = device->getSensor(nplane);
    // Apply alignment to hits
    for (unsigned int nhit = 0; nhit < plane->getNumHits(); nhit++) {
      Storage::Hit* hit = plane->getHit(nhit);
      double posX = 0, posY = 0, posZ = 0;
      // std::cout << "At plane: " << nplane << std::endl;
      sensor->pixelToSpace(
          hit->getPixX() + 0.5, hit->getPixY() + 0.5, posX, posY, posZ);
      hit->setPos(posX, posY, posZ);
      //  cout<<"Sensor= "<<nplane  <<"in processors,hit , set pos
      //  Z="<<posZ<<std::endl;
    }

    // Apply alignment to clusters
    for (unsigned int ncluster = 0; ncluster < plane->getNumClusters();
         ncluster++) {
      Storage::Cluster* cluster = plane->getCluster(ncluster);
      double posX = 0, posY = 0, posZ = 0;
      // std::cout << "At plane: " << nplane << std::endl;
      sensor->pixelToSpace(
          cluster->getPixX(), cluster->getPixY(), posX, posY, posZ);
      cluster->setPos(posX, posY, posZ);
      // cout<<"Sensor= "<<nplane  <<"in processors,cluster , set pos
      // Z="<<posZ<<std::endl;

      XYZVector errLocal(sensor->getPitchX() * cluster->getPixErrX(),
                         sensor->getPitchY() * cluster->getPixErrY(),
                         0);
      XYZVector errGlobal = sensor->localToGlobal() * errLocal;
      cluster->setPosErr(
          fabs(errGlobal.x()), fabs(errGlobal.y()), fabs(errGlobal.z()));
    }
  }

  // Apply alignment to tracks
  for (unsigned int ntrack = 0; ntrack < event->getNumTracks(); ntrack++) {
    Storage::Track* track = event->getTrack(ntrack);
    Processors::TrackMaker::fitTrackToClusters(track);
  }
}
