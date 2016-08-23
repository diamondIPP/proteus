#include "coarsealign.h"

#include <cassert>
#include <vector>

#include <Rtypes.h>
#include <TH1D.h>

#include "../storage/storageio.h"
#include "../storage/event.h"
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
#include "../mechanics/alignment.h"
#include "../processors/processors.h"
#include "../processors/clustermaker.h"
#include "../analyzers/singleanalyzer.h"
#include "../analyzers/dualanalyzer.h"
#include "../analyzers/correlation.h"

using std::cout;
using std::endl;

//=========================================================
Loopers::CoarseAlign::CoarseAlign(Mechanics::Device* refDevice,
				  Processors::ClusterMaker* clusterMaker,
				  Storage::StorageIO* refInput,
				  ULong64_t startEvent,
				  ULong64_t numEvents,
				  unsigned int eventSkip) :
  Looper(refInput, 0, startEvent, numEvents, eventSkip),
  _refDevice(refDevice),
  _clusterMaker(clusterMaker),
  _displayFits(true)
{
  assert(refInput && refDevice && clusterMaker &&
	 "Looper: initialized with null object(s)");
  
  assert(refInput->getNumPlanes() == refDevice->getNumSensors() &&
	 "Loopers: number of planes / sensors mis-match");
  
  std::cout << "CoarseAlign::CoarseAlign" << std::endl;
  refDevice->print();
}

//=========================================================
void Loopers::CoarseAlign::setDisplayFits(bool value) {
  _displayFits = value;
}

//=========================================================
void Loopers::CoarseAlign::loop(){
  // Coarse align specific analyzers
  Analyzers::Correlation correlation(_refDevice, 0); // 0  for no output
  
  for (ULong64_t nevent=_startEvent; nevent<=_endEvent; nevent++)      {
    Storage::Event* refEvent = _refStorage->readEvent(nevent);
    
    if (refEvent->getNumClusters())
      throw "CoarseAlign: can't recluster an event, mask the tree in the input";
    
    for (unsigned int nplane=0; nplane<refEvent->getNumPlanes(); nplane++)
      _clusterMaker->generateClusters(refEvent, nplane);
    
    Processors::applyAlignment(refEvent, _refDevice);
    
    correlation.processEvent(refEvent);
    
    progressBar(nevent);
    
    delete refEvent;
  }
  
  double cummulativeX = 0;
  double cummulativeY = 0;

  Mechanics::Alignment& align = *_refDevice->getAlignment();

  for(unsigned int nsensor=1; nsensor<_refDevice->getNumSensors(); nsensor++){
    Mechanics::Sensor* sensor = _refDevice->getSensor(nsensor);
    
    TH1D* alignX = correlation.getAlignmentPlotX(nsensor);
    double offsetX = 0;
    double sigmaX = 0;
    Processors::fitGaussian(alignX, offsetX, sigmaX, _displayFits);
    cummulativeX -= offsetX;
    
    TH1D* alignY = correlation.getAlignmentPlotY(nsensor);
    double offsetY = 0;
    double sigmaY = 0;
    Processors::fitGaussian(alignY, offsetY, sigmaY, _displayFits);
    cummulativeY -= offsetY;
    
    std::cout << "For sensor: " << nsensor << std::endl;
    std::cout << "Gaussian mean: X= " << offsetX << "  Y= " << offsetY << std::endl;
    std::cout << "Cummulative    X= " << cummulativeX << "  Y= " << cummulativeY << std::endl;

    align.correctOffset(nsensor, cummulativeX, cummulativeY, 0);

    std::cout << "New offset: X= " << sensor->getOffX() << "  Y= " << sensor->getOffY() << std::endl;
  }
  
  // create output alignment file
  align.writeFile();
}

//=========================================================
void Loopers::CoarseAlign::print() const {
  cout << "## [CoarseAlign::print]" << endl;
  cout << "  - display fits: " << _displayFits << endl;
}
