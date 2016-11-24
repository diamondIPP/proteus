#include "coarsealigndut.h"

#include <cassert>
#include <vector>

#include <Rtypes.h>

#include "../storage/storageio.h"
#include "../storage/event.h"
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
#include "../mechanics/alignment.h"
#include "../processors/applyalignment.h"
#include "../processors/clustermaker.h"
#include "../processors/processors.h"
#include "../analyzers/singleanalyzer.h"
#include "../analyzers/dualanalyzer.h"
#include "../analyzers/dutcorrelation.h"

using std::cout;
using std::endl;

//=========================================================
Loopers::CoarseAlignDut::CoarseAlignDut(Mechanics::Device* refDevice,
					Mechanics::Device* dutDevice,
					Processors::ClusterMaker* clusterMaker,
					Storage::StorageIO* refInput,
					Storage::StorageIO* dutInput,
					ULong64_t startEvent,
					ULong64_t numEvents,
					unsigned int eventSkip) :
  Looper(refInput, dutInput, startEvent, numEvents, eventSkip),
  _refDevice(refDevice),
  _dutDevice(dutDevice),
  _clusterMaker(clusterMaker),
  _displayFits(true)
{
  assert(refInput && dutInput && refDevice && dutDevice && clusterMaker &&
         "Looper: initialized with null object(s)");
  assert(refInput->getNumPlanes() == refDevice->getNumSensors() &&
         dutInput->getNumPlanes() == dutDevice->getNumSensors() &&
         "Loopers: number of planes / sensors mis-match");
}

//=========================================================
void Loopers::CoarseAlignDut::setDisplayFits(bool value) {
  _displayFits = value;
}

//=========================================================
void Loopers::CoarseAlignDut::loop()
{
  Analyzers::DUTCorrelation correlation(_refDevice, _dutDevice, 0);

  for (ULong64_t nevent = _startEvent; nevent <= _endEvent; nevent++)
  {
    Storage::Event* refEvent = _refStorage->readEvent(nevent);
    Storage::Event* dutEvent = _dutStorage->readEvent(nevent);

    if (refEvent->getNumClusters() || dutEvent->getNumClusters())
      throw "CoarseAlignDut: can't recluster an event, mask the tree in the input";
    for (unsigned int nplane = 0; nplane < refEvent->getNumPlanes(); nplane++)
      _clusterMaker->generateClusters(refEvent, nplane);
    for (unsigned int nplane = 0; nplane < dutEvent->getNumPlanes(); nplane++)
      _clusterMaker->generateClusters(dutEvent, nplane);

    Processors::setGeometry(refEvent, _refDevice);
    Processors::setGeometry(dutEvent, _dutDevice);

    correlation.processEvent(refEvent, dutEvent);

    progressBar(nevent);

    delete refEvent;
    delete dutEvent;
  }

  Mechanics::Alignment newAlignment = _dutDevice->geometry();

  for (unsigned int nsensor = 0; nsensor < _dutDevice->getNumSensors(); nsensor++)
  {
    Mechanics::Sensor* sensor = _dutDevice->getSensor(nsensor);

    TH1D* alignX = correlation.getAlignmentPlotX(nsensor);
    double offsetX = 0;
    double sigmaX = 0;
    Processors::fitGaussian(alignX, offsetX, sigmaX, _displayFits);

    TH1D* alignY = correlation.getAlignmentPlotY(nsensor);
    double offsetY = 0;
    double sigmaY = 0;
    Processors::fitGaussian(alignY, offsetY, sigmaY, _displayFits);
   std::cout << "Old offset: X= " << sensor->getOffX() << "  Y= " << sensor->getOffY() << std::endl;

   newAlignment.correctGlobalOffset(nsensor, -offsetX, -offsetY, 0);

   std::cout << "DUT plane: " << nsensor << std::endl;
   std::cout << "gaussian mean: X= " << offsetX << "  Y= " << offsetY << std::endl;
   std::cout << "New offset: X= " << sensor->getOffX() << "  Y= " << sensor->getOffY() << std::endl;
  }

  newAlignment.writeFile(_dutDevice->pathAlignment());
}

//=========================================================
void Loopers::CoarseAlignDut::print() const {
  cout << "## [CoarseAlignDut::print]" << endl;
  cout << "  - display fits: " << _displayFits << endl;
}
