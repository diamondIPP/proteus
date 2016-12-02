#include "finealigndut.h"

#include <cassert>
#include <vector>
#include <iostream>

#include <Rtypes.h>

#include "../storage/storageio.h"
#include "../storage/event.h"
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
#include "../mechanics/geometry.h"
#include "../processors/applygeometry.h"
#include "../processors/processors.h"
#include "../processors/clustermaker.h"
#include "../processors/trackmaker.h"
#include "../analyzers/singleanalyzer.h"
#include "../analyzers/dualanalyzer.h"
#include "../analyzers/cuts.h"
#include "../analyzers/dutresiduals.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

using std::cout;
using std::endl;

//=========================================================
Loopers::FineAlignDut::FineAlignDut(Mechanics::Device* refDevice,
				    Mechanics::Device* dutDevice,
				    Processors::ClusterMaker* clusterMaker,
				    Processors::TrackMaker* trackMaker,
				    Storage::StorageIO* refInput,
				    Storage::StorageIO* dutInput,
				    ULong64_t startEvent,
				    ULong64_t numEvents,
				    unsigned int eventSkip) :
  Looper(refInput, dutInput, startEvent, numEvents, eventSkip),
  _refDevice(refDevice),
  _dutDevice(dutDevice),
  _clusterMaker(clusterMaker),
  _trackMaker(trackMaker),
  _numIterations(5),
  _numBinsY(15),
  _numPixX(5),
  _binsPerPix(10),
  _numPixXBroad(20),
  _binsPerPixBroad(1),
  _displayFits(true),
  _relaxation(0.8)
{
  assert(refInput && dutInput && refDevice && dutDevice && clusterMaker && trackMaker &&
         "Looper: initialized with null object(s)");
  assert(refInput->getNumPlanes() == refDevice->getNumSensors() &&
         dutInput->getNumPlanes() == dutDevice->getNumSensors() &&
         "Loopers: number of planes / sensors mis-match");
}

//=========================================================
void Loopers::FineAlignDut::loop()
{
  Mechanics::Geometry newAlignment = _dutDevice->geometry();

  for (unsigned int niter = 0; niter < _numIterations; niter++)
  {
    cout << "Iteration " << niter << " of " << _numIterations - 1 << endl;

    // Use broad residual resolution for the first iteration
    const unsigned int numPixX = (niter == 0) ? _numPixXBroad : _numPixX;
    const double binxPerPix = (niter == 0) ? _binsPerPixBroad : _binsPerPix;

    // Residuals of DUT clusters to ref. tracks
    Analyzers::DUTResiduals residuals(_refDevice, _dutDevice, 0, "", numPixX, binxPerPix, _numBinsY);

    // Use events with only 1 track
    Analyzers::Cuts::EventTracks* cut1 =
        new Analyzers::Cuts::EventTracks(1, Analyzers::EventCut::EQ);

    // Use tracks with one hit in each plane
    const unsigned int numClusters = _refDevice->getNumSensors();
    Analyzers::Cuts::TrackClusters* cut2 =
        new Analyzers::Cuts::TrackClusters(numClusters, Analyzers::TrackCut::EQ);

    // Use tracks with low chi2
//    Analyzers::Cuts::TrackClusters* cut3 =
//        new Analyzers::Cuts::TrackChi2(1, Analyzers::TrackCut::LT);

    // Note: the analyzer will delete the cuts
    residuals.addCut(cut1);
    residuals.addCut(cut2);

    _trackMaker->setBeamSlope(_refDevice->getBeamSlopeX(),
                              _refDevice->getBeamSlopeY());

    for (ULong64_t nevent = _startEvent; nevent <= _endEvent; nevent++)
    {
      Storage::Event* refEvent = _refStorage->readEvent(nevent);
      Storage::Event* dutEvent = _dutStorage->readEvent(nevent);

      if (refEvent->getNumClusters() || dutEvent->getNumClusters())
        throw "FineAlignDut: can't recluster an event, mask the tree in the input";
      for (unsigned int nplane = 0; nplane < refEvent->getNumPlanes(); nplane++)
        _clusterMaker->generateClusters(refEvent, nplane);
      for (unsigned int nplane = 0; nplane < dutEvent->getNumPlanes(); nplane++)
        _clusterMaker->generateClusters(dutEvent, nplane);

      Processors::applyAlignment(refEvent, _refDevice);
      Processors::applyAlignment(dutEvent, _dutDevice);

      if (refEvent->getNumTracks())
        throw "FineAlign: can't re-track an event, mask the tree in the input";
      _trackMaker->generateTracks(refEvent);

      residuals.processEvent(refEvent, dutEvent);

      progressBar(nevent);

      delete refEvent;
      delete dutEvent;
    }

    for (unsigned int nsens = 0; nsens < _dutDevice->getNumSensors(); nsens++)
    {
      //std::cout << "sensor #:" << nsens << std::endl;
      Mechanics::Sensor* sensor = _dutDevice->getSensor(nsens);

      double offsetX = 0, offsetY = 0, rotation = 0;

       //Bilbao@cern.ch: first alignment using the residuals in order to avoid a big ofset on the 2D technique. This also helps since the DUT is align with repspect to a ref plane but not consdeing the cumulative shift.
 
      if(niter==0){
      double sigmaX = 0, maxX = 0, backgroundX = 0;
      double sigmaY = 0, maxY = 0, backgroundY = 0;
      Processors::fitGaussian(residuals.getResidualX(nsens), offsetX, sigmaX, maxX, backgroundX, _displayFits);
      Processors::fitGaussian(residuals.getResidualY(nsens), offsetY, sigmaY, maxX, backgroundY, _displayFits);
      
      std::cout << "Fine alignment with residuals:" << std::endl;
      std::cout << "   Sensor: " << nsens << ", Xcorrection: " << offsetX << ", Ycorrection: " << offsetY <<  std::endl;
      newAlignment.correctGlobalOffset(nsens, offsetX, offsetY, 0);
      newAlignment.correctRotationAngles(nsens, 0, 0, rotation);
      offsetX=0;
      offsetY=0;
      rotation=0;
      }
      
      Processors::residualAlignment(residuals.getResidualXY(nsens),
                                    residuals.getResidualYX(nsens),
                                    offsetX, offsetY, rotation, _displayFits);
      
      std::cout << "Fine alignment with 2D slicing:" << std::endl;
      std::cout << "   Sensor: " << nsens << ", "
		<< "Xcorrection: " << offsetX << ", "
		<< "Ycorrection: " << offsetY << ", "
		<< "Zcorrection: " << rotation
		<< std::endl;
			newAlignment.correctGlobalOffset(nsens, offsetX, offsetY, 0);
			newAlignment.correctRotationAngles(nsens, 0, 0, rotation);
      std::cout << "Sensor: " << nsens << ", "
		<< "Xoffset: " << sensor->getOffX() << ", "
		<< "Yoffset: " << sensor->getOffY() << ", "
		<< "Zoffset: " << sensor->getOffZ()
		<< std::endl;
      std::cout << std::endl;

      if(niter==0 && _dutDevice->getNumSensors()== 1 && offsetX==0 && offsetY==0){
	std::cout << "The fine alignment was terminated!" << std::endl;
	std::cout << "Only one DUT is present and 2D residuals is not working!" << std::endl;
	niter=_numIterations+1;
      }
    }

  }

  newAlignment.writeFile(_dutDevice->pathGeometry());
}

void Loopers::FineAlignDut::setNumIterations(unsigned int value) { _numIterations = value; }
void Loopers::FineAlignDut::setNumBinsY(unsigned int value) { _numBinsY = value; }
void Loopers::FineAlignDut::setNumPixX(unsigned int value) { _numPixX = value; }
void Loopers::FineAlignDut::setBinsPerPix(double value) { _binsPerPix = value; }
void Loopers::FineAlignDut::setNumPixXBroad(unsigned int value) { _numPixXBroad = value; }
void Loopers::FineAlignDut::setBinsPerPixBroad(double value) { _binsPerPixBroad = value; }
void Loopers::FineAlignDut::setDisplayFits(bool value) { _displayFits = value; }
void Loopers::FineAlignDut::setRelaxation(double value) { _relaxation = value; }

void Loopers::FineAlignDut::print() const {
  cout << "\n## [FineAlignDut::print]" << endl;
  cout << "  - numIterations   : "  << _numIterations << endl;
  cout << "  - numBinsY        : "  << _numBinsY << endl;
  cout << "  - numPixX         : "  << _numPixX << endl;
  cout << "  - binsPerPix      : "  << _binsPerPix << endl;
  cout << "  - numPixXBroad    : "  << _numPixXBroad << endl;
  cout << "  - binsPerPixBroad : "  << _binsPerPixBroad << endl;
  cout << "  - display Fits    : "  << _displayFits << endl;
  cout << "  - relaxation      : "  << _relaxation << endl;
}
