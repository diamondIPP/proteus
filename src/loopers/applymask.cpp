#include "applymask.h"

#include <cassert>
#include <vector>
#include <iostream>

#include <Rtypes.h>

#include "../storage/storageio.h"
#include "../storage/event.h"
#include "../storage/plane.h"
#include "../storage/hit.h"
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

using std::cout;
using std::endl;
using std::flush;

namespace Loopers {

  //=========================================================
  ApplyMask::ApplyMask(Mechanics::Device* refDevice,
		       Storage::StorageIO* refOutput,
		       Storage::StorageIO* refInput,
		       ULong64_t startEvent,
		       ULong64_t numEvents,
		       unsigned int eventSkip) :
    Looper(refInput, 0, startEvent, numEvents, eventSkip),
    _refDevice(refDevice),
    _refOutput(refOutput)
  {
    assert(refInput && refDevice && refOutput &&
	   "Looper: initialized with null object(s)");

    assert(refInput->getNumPlanes() == refDevice->getNumSensors() &&
	   "Loopers: number of planes / sensors mis-match");
  }
  
  //=========================================================
  void ApplyMask::loop() {

    for (ULong64_t nevent=_startEvent; nevent<=_endEvent; nevent++){
      Storage::Event* refEvent = _refStorage->readEvent(nevent);
      Storage::Event* maskedEvent = new Storage::Event(_refDevice->getNumSensors());

      for (Index iplane = 0; iplane < refEvent->numPlanes(); ++iplane) {
        const Storage::Plane* plane = refEvent->getPlane(iplane);
        Storage::Plane* maskedPlane = maskedEvent->getPlane(iplane);
        for (Index ihit = 0; ihit < plane->numHits(); ihit++) {
          const Storage::Hit* hit = plane->getHit(ihit);
          const Index col = hit->col();
          const Index row = hit->row();
          if (!_refDevice->getSensor(iplane)->isPixelNoisy(col, row)) {
            Storage::Hit* copy = maskedPlane->newHit();
            copy->setAddress(col, row);
            copy->setTime(hit->time());
            copy->setValue(hit->value());
          }
        }
      }

      maskedEvent->setTimestamp(refEvent->getTimeStamp());
      maskedEvent->setFrameNumber(refEvent->getFrameNumber());
      maskedEvent->setTriggerOffset(refEvent->getTriggerOffset());
      maskedEvent->setTriggerInfo(refEvent->getTriggerInfo());
      maskedEvent->setTriggerPhase(refEvent->getTriggerPhase());
      maskedEvent->setInvalid(refEvent->getInvalid());
      
      // Write the event
      _refOutput->writeEvent(maskedEvent);
      
      progressBar(nevent);
      
      delete refEvent;
      delete maskedEvent;

    } // end loop in events
  }
  
} // end of namespace
