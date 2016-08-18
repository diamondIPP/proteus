#include "finealign.h"

#include <cassert>
#include <vector>
#include <iostream>
#include <algorithm>

#include <Rtypes.h>

#include "../storage/storageio.h"
#include "../storage/event.h"
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
#include "../mechanics/alignment.h"
#include "../processors/processors.h"
#include "../processors/clustermaker.h"
#include "../processors/trackmaker.h"
#include "../analyzers/singleanalyzer.h"
#include "../analyzers/dualanalyzer.h"
#include "../analyzers/cuts.h"
#include "../analyzers/residuals.h"

#include "TGraphErrors.h"
#include "TFile.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

using std::cout;
using std::endl;

//=========================================================
Loopers::FineAlign::FineAlign(Mechanics::Device* refDevice,
			      Processors::ClusterMaker* clusterMaker,
			      Processors::TrackMaker* trackMaker,
			      Storage::StorageIO* refInput,
			      ULong64_t startEvent,
			      ULong64_t numEvents,
			      unsigned int eventSkip) :
  Looper(refInput, 0, startEvent, numEvents, eventSkip),
  _refDevice(refDevice),
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
  assert(refInput && refDevice && clusterMaker && trackMaker &&
         "Looper: initialized with null object(s)");
  assert(refInput->getNumPlanes() == refDevice->getNumSensors() &&
         "Loopers: number of planes / sensors mis-match");
}

//=========================================================
void Loopers::FineAlign::loop()
{
  // Build a vector of sensor indices which will be permutated at eaich iteration
  //fdibello@cern.ch convergence plot for the fine aligmenet variables, offset x=0, offset y=1, offset z=2, rotation z=3
  TGraphErrors *convergence[4][_refDevice->getNumSensors()];
  float nit[_numIterations];
  float ofx[_numIterations][_refDevice->getNumSensors()];
  float ofy[_numIterations][_refDevice->getNumSensors()];
  float ofz[_numIterations][_refDevice->getNumSensors()];
  float rotz[_numIterations][_refDevice->getNumSensors()];
  float sigmax[_numIterations][_refDevice->getNumSensors()];
  float sigmay[_numIterations][_refDevice->getNumSensors()];
  
  TFile *out_file = new TFile("aligment_convergence_DUT_C22_masking_plane0.root","RECREATE");
  TDirectory *sensordir[_refDevice->getNumSensors()];
  
  double rotation1[_refDevice->getNumSensors()]; //fdibello offset in the aligmnent plot

  for(unsigned int i=0;i<_refDevice->getNumSensors() ;i++){
    for(unsigned int l=0;l< _numIterations;l++){
      ofx[l][i]=0;
      ofy[l][i]=0;
      ofz[l][i]=0;
      rotz[l][i]=0; //rotzz[l]=rotz[l][i]-rotation1[i];
    }
  }

  for (unsigned int f=0; f<_refDevice->getNumSensors(); f++){
    sensordir[f] = out_file->mkdir(Form("sensor_%d",f));
  }
  
  std::vector<unsigned int> sensorPermutations(_refDevice->getNumSensors(), 0);
  for (unsigned int i = 0; i < _refDevice->getNumSensors(); i++)
    sensorPermutations[i] = i;
  // Start with permutation so that the first itertaion permutes back to the
  // ordered list (just looks less strange if the first iteration is ordered)
  std::prev_permutation(sensorPermutations.begin(), sensorPermutations.end());

  for (unsigned int niter = 0; niter < _numIterations; niter++)
  {
    cout << "Iteration " << niter << " of " << _numIterations - 1 << endl;

    // Get the average slopes to make device rotations
    double avgSlopeX = 0;
    double avgSlopeY = 0;
    ULong64_t numSlopes = 0;

    // Permute the order in which the sensors will be processed
    std::next_permutation(sensorPermutations.begin(), sensorPermutations.end());

    // Each sensor gets an unbiased residual run, and there is an extra run for overall alignment
    for (unsigned int nsensor = 0; nsensor < _refDevice->getNumSensors(); nsensor++)
    {
      // Use sensor index from the permutated list of indices
      const unsigned int nsens = sensorPermutations[nsensor];

      cout << "Sensor " << nsens << endl;

      if(nsens==0) continue; //fdibello in trackmaker nsens is the masked plane

      Mechanics::Sensor* sensor = _refDevice->getSensor(nsens);

      // Use broad residual resolution for the first iteration
      const unsigned int numPixX = (niter == 0) ? _numPixXBroad : _numPixX;
      const double binxPerPix = (niter == 0) ? _binsPerPixBroad : _binsPerPix;

      Analyzers::Residuals residuals(_refDevice, 0, "", numPixX, binxPerPix, _numBinsY);

      // Use events with only 1 track
      Analyzers::Cuts::EventTracks* cut1 =
          new Analyzers::Cuts::EventTracks(1, Analyzers::Cut::EQ);

      // Use tracks with one hit in each plane (one is masked)
      const unsigned int numClusters = _refDevice->getNumSensors() - 1;
      Analyzers::Cuts::TrackClusters* cut2 =
          new Analyzers::Cuts::TrackClusters(numClusters, Analyzers::Cut::EQ);

      // Note: the analyzer will delete the cuts
      residuals.addCut(cut1);
      residuals.addCut(cut2);

      for (ULong64_t nevent = _startEvent; nevent <= _endEvent; nevent++)
      {
        Storage::Event* refEvent = _refStorage->readEvent(nevent);

        if (refEvent->getNumClusters())
          throw "FineAlign: can't recluster an event, mask the tree in the input";
        for (unsigned int nplane = 0; nplane < refEvent->getNumPlanes(); nplane++)
          _clusterMaker->generateClusters(refEvent, nplane);

        Processors::applyAlignment(refEvent, _refDevice);

        if (refEvent->getNumTracks())
          throw "FineAlign: can't re-track an event, mask the tree in the input";
        _trackMaker->generateTracks(refEvent,
                                    _refDevice->getBeamSlopeX(),
                                    _refDevice->getBeamSlopeY(),
                                    nsens);

        // For the average track slopes
        for (unsigned int ntrack = 0; ntrack < refEvent->getNumTracks(); ntrack++)
        {
          Storage::Track* track = refEvent->getTrack(ntrack);
          avgSlopeX += track->getSlopeX();
          avgSlopeY += track->getSlopeY();
          numSlopes++;
        }

        residuals.processEvent(refEvent);

        progressBar(nevent);

        delete refEvent;
      }

      double offsetX = 0, offsetY = 0, rotation = 0;


      //Bilbao@cern.ch: first alignment using the residuals in order to avoid a big ofset on the 2D technique. This also helps since the DUT is align with repspect to a ref plane but not consdeing the cumulative shift.

      if(niter==0){
      double sigmaX = 0, maxX = 0, backgroundX = 0;
      double sigmaY = 0, maxY = 0, backgroundY = 0;
      Processors::fitGaussian(residuals.getResidualX(nsens), offsetX, sigmaX, maxX, backgroundX, _displayFits);
      Processors::fitGaussian(residuals.getResidualY(nsens), offsetY, sigmaY, maxX, backgroundY, _displayFits);

      std::cout << "Fine alignment with residuals:" << std::endl;
      std::cout << "   Sensor: " << nsens << ", Xcorrection: " << offsetX << ", Ycorrection: " << offsetY <<  std::endl;
			// TODO msmk 2016-08-18 switch to new alignment
      // sensor->setOffX(sensor->getOffX() + offsetX); //fdibello-->I think this is useless
      // sensor->setOffY(sensor->getOffY() + offsetY);
      // sensor->setRotZ(sensor->getRotZ() + rotation);
      offsetX=0;
      offsetY=0;
      rotation=0;
      }

      Processors::residualAlignment(residuals.getResidualXY(nsens), //i guess this is a typo --->nsens below instead of nsensor
                                    residuals.getResidualYX(nsens),
                                    offsetX, offsetY, rotation,
                                    _relaxation, _displayFits);

      if(niter==0){rotation1[nsens]=rotation;}  //fdibello offset for the ali. plot

      std::cout << "Sensor: " << nsens << ", Xcorrection: " << offsetX<< ", Ycorrection: " << offsetY << ", Zcorrection: " << rotation << std::endl;
			// TODO msmk 2016-08-18 switch to new alignment
      // sensor->setOffX(sensor->getOffX() + offsetX);
      // sensor->setOffY(sensor->getOffY() + offsetY);
      // sensor->setRotZ(sensor->getRotZ() + rotation);
      // sensor->setRotX(sensor->getRotX()); //fdibello
      // std::cout << "Sensor: " << nsens << ", Xoffset: " << sensor->getOffX()<< ", Yoffset: " << sensor->getOffY() << ", Zoffset: " << sensor->getOffZ() <<",  RotationX="<<sensor->getRotX()<<",  RotationY="<<sensor->getRotY()<<",  RotationZ="<<sensor->getRotZ()<< std::endl;
     
    //fdibello
    ofx[niter][nsens]=sensor->getOffX();
    ofy[niter][nsens]=sensor->getOffY();
    ofz[niter][nsens]=sensor->getOffZ();
		// TODO msmk 2016-08-18 switch to new alignment
    // rotz[niter][nsens]=sensor->getRotZ();
    nit[niter]=niter;
     
    //  double sigmaX1 = 0, maxX1 = 0, backgroundX1 = 0;
   //   double sigmaY1 = 0, maxY1 = 0, backgroundY1 = 0;
 
   // Processors::fitGaussian(residuals.getResidualX(nsens), offsetX, sigmaX1, maxX1,backgroundX1, _displayFits);
   // Processors::fitGaussian(residuals.getResidualY(nsens), offsetY, sigmaY1, maxX1,backgroundY1, _displayFits);
   // sigmax[niter][nsens]=sigmaX1;
   // sigmay[niter][nsens]=sigmaY1;
    
    }
    // Ajudst the device rotation using the average slopes
    avgSlopeX /= (double)numSlopes;
    avgSlopeY /= (double)numSlopes;
    _refDevice->setBeamSlopeX(_refDevice->getBeamSlopeX() + avgSlopeX);
    _refDevice->setBeamSlopeY(_refDevice->getBeamSlopeY() + avgSlopeY);

    cout << endl; // Space between iterations
  }
   //fdibello
   for(unsigned int i=0;i<_refDevice->getNumSensors() ;i++){
 //  float sigmaxx[_numIterations];
 //  float sigmayy[_numIterations];
   float ofxx[_numIterations];
   float ofyy[_numIterations];
   float rotzz[_numIterations];
   float ofzz[_numIterations];
   for(unsigned int l=0;l< _numIterations;l++){
 //  sigmaxx[l]=sigmax[l][i];
 //  sigmayy[l]=sigmay[l][i];
   ofxx[l]=ofx[l][i];
   ofyy[l]=ofy[l][i];
   ofzz[l]=ofz[l][i];
   rotzz[l]=rotz[l][i]; //rotzz[l]=rotz[l][i]-rotation1[i];
    }

    convergence[0][i] = new TGraphErrors(_numIterations,nit,ofxx,0,0); //how estimate error for the rotation?
    convergence[1][i] = new TGraphErrors(_numIterations,nit,ofyy,0,0); //how estimate error for the rotation?
    convergence[2][i] = new TGraphErrors(_numIterations,nit,ofzz,0,0); //how estimate error for the rotation?
    convergence[3][i] = new TGraphErrors(_numIterations,nit,rotzz,0,0); //how estimate error for the rotation?

   sensordir[i]->cd();
   convergence[0][i]->GetYaxis()->SetTitle("offset_X [#mum]");
   convergence[1][i]->GetYaxis()->SetTitle("offset_Y [#mum]");
   convergence[2][i]->GetYaxis()->SetTitle("offset_Z [#mum]");
   convergence[3][i]->GetYaxis()->SetTitle("rot_Z [#mum]");


   convergence[0][i]->GetXaxis()->SetTitle("# iteration");
   convergence[1][i]->GetXaxis()->SetTitle("# iteration");
   convergence[2][i]->GetXaxis()->SetTitle("# iteration");
   convergence[3][i]->GetXaxis()->SetTitle("# iteration");



   convergence[0][i]->Write();
   convergence[1][i]->Write();
   convergence[2][i]->Write();
   convergence[3][i]->Write();


   }
  out_file->Close();
  _refDevice->getAlignment()->writeFile();
}

void Loopers::FineAlign::setNumIterations(unsigned int value) { _numIterations = value; }
void Loopers::FineAlign::setNumBinsY(unsigned int value) { _numBinsY = value; }
void Loopers::FineAlign::setNumPixX(unsigned int value) { _numPixX = value; }
void Loopers::FineAlign::setBinsPerPix(double value) { _binsPerPix = value; }
void Loopers::FineAlign::setNumPixXBroad(unsigned int value) { _numPixXBroad = value; }
void Loopers::FineAlign::setBinsPerPixBroad(double value) { _binsPerPixBroad = value; }
void Loopers::FineAlign::setDisplayFits(bool value) { _displayFits = value; }
void Loopers::FineAlign::setRelaxation(double value) { _relaxation = value; }

void Loopers::FineAlign::print() const {
  cout << "\n## [FineAlign::print]" << endl;
  cout << "  - numIterations   : "  << _numIterations << endl;
  cout << "  - numBinsY        : "  << _numBinsY << endl;
  cout << "  - numPixX         : "  << _numPixX << endl;
  cout << "  - binsPerPix      : "  << _binsPerPix << endl;
  cout << "  - numPixXBroad    : "  << _numPixXBroad << endl;
  cout << "  - binsPerPixBroad : "  << _binsPerPixBroad << endl;
  cout << "  - display Fits    : "  << _displayFits << endl;
  cout << "  - relaxation      : "  << _relaxation << endl;
}
