#include "depiction.h"

#include <cassert>
#include <math.h>
#include <sstream>
#include <vector>

#include <TDirectory.h>
#include <TH1D.h>
#include <TH2D.h>

// Access to the device being analyzed and its sensors
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
// Access to the data stored in the event
#include "../storage/cluster.h"
#include "../storage/event.h"
#include "../storage/hit.h"
#include "../storage/plane.h"
#include "../storage/track.h"
// Some generic processors to calcualte typical event related things
#include "../processors/eventdepictor.h"
#include "../processors/processors.h"
// This header defines all the cuts
#include "cuts.h"

namespace Analyzers {

void Depictor::processEvent(const Storage::Event* refEvent)
{
  assert(refEvent && "Analyzer: can't process null events");

  if (_depictEvent) {
    // Check if the event passes the cuts
    if (!checkCuts(refEvent))
      return;

    _depictor->depictEvent(refEvent, 0);
  }

  if (_depictClusters) {
    std::vector<const Storage::Cluster*> refClusters;
    for (Index iplane = 0; iplane < refEvent->numPlanes(); ++iplane) {
      const Storage::Plane* plane = refEvent->getPlane(iplane);
      for (Index icluster = 0; icluster < plane->numClusters(); icluster++) {
        const Storage::Cluster* cluster = plane->getCluster(icluster);

        // Check if the cluster passes the cuts
        if (!checkCuts(cluster))
          continue;

        refClusters.push_back(cluster);
      }
    }

    std::vector<const Storage::Cluster*> dutClusters;

    _depictor->depictClusters(refClusters, dutClusters);
  }

  if (_depictTracks) {
    for (unsigned int ntrack = 0; ntrack < refEvent->numTracks(); ntrack++) {
      const Storage::Track* track = refEvent->getTrack(ntrack);

      // Check if the track passes the cuts
      if(!checkCuts(track))
        continue;

      _depictor->depictTrack(track);
    }
  }
}

void Depictor::postProcessing() {} // Needs to be declared even if not used

Depictor::Depictor(const Mechanics::Device* refDevice,
                   TDirectory* dir,
                   const char* suffix,
                   bool depictEvent,
                   bool depictClusters,
                   bool depictTracks,
                   double zoom)
    : // Base class is initialized here and manages directory / device
    SingleAnalyzer(refDevice, dir, suffix)
    , _depictEvent(depictEvent)
    , _depictClusters(depictClusters)
    , _depictTracks(depictTracks)
    , _depictor(0)
{
  _depictor = new Processors::EventDepictor(_device, 0);
  _depictor->setZoom(zoom);
}
}
