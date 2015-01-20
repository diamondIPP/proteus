#include "noisescan.h"

#include <cassert>
#include <vector>
#include <iostream>
#include <algorithm>

#include <Rtypes.h>
#include <TCanvas.h>
#include <TH1D.h>

#include "../storage/storageio.h"
#include "../storage/event.h"
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
#include "../mechanics/noisemask.h"
#include "../processors/processors.h"
#include "../processors/clustermaker.h"
#include "../processors/trackmaker.h"
#include "../analyzers/singleanalyzer.h"
#include "../analyzers/dualanalyzer.h"
#include "../analyzers/occupancy.h"

using std::cout;
using std::endl;
using std::cin;

//=========================================================
Loopers::NoiseScan::NoiseScan(Mechanics::Device* refDevice,
                     Storage::StorageIO* refInput,
                     ULong64_t startEvent,
                     ULong64_t numEvents,
                     unsigned int eventSkip) :
  Looper(refInput, 0, startEvent, numEvents, eventSkip),
  _refDevice(refDevice),
  _maxFactor(10),
  _maxOccupancy(0),
  _bottomLimitX(-1),
  _upperLimitX(-1),
  _bottomLimitY(-1),
  _upperLimitY(-1)
{
  assert(refInput && refDevice && "Looper: initialized with null object(s)");
  assert(refInput->getNumPlanes() == refDevice->getNumSensors() &&
         "Loopers: number of planes / sensors mis-match");
}

//=========================================================
void Loopers::NoiseScan::loop(){

  Analyzers::Occupancy occupancy(_refDevice, 0);

  int totEvt=0;
  for (ULong64_t nevent=_startEvent; nevent<=_endEvent; nevent++, totEvt++){
    Storage::Event* refEvent = _refStorage->readEvent(nevent);    
    occupancy.processEvent(refEvent);    
    progressBar(nevent);    
    delete refEvent;
  }
  cout << "nevents = "<< totEvt << endl;
  occupancy.postProcessing();

  cout << "totalHitOccupancy = "<< occupancy.totalHitOccupancy << endl;

  //cout << "Find a cutoff occupancy from the following, then close the window." << endl;
  //TCanvas* c = new TCanvas("OccCan", "Occupancy Canvas", 600, 600);
  //c->SetLogx();
  //c->SetLogy();
  //TH1D* hist = occupancy.getOccDistribution();
  //hist->Draw();
  //gPad->Modified();
  //c->WaitPrimitive();
  
  //  double cutoff = 0;
  //  unsigned int noisyPixels = 0;
  
  //  while (true)
  //  {
  //    cout << "Enter maximal occupancy: ";
  //    cin >> cutoff;
  
  //    noisyPixels = 0;  
  //    for (unsigned int nsens = 0; nsens < _refDevice->getNumSensors(); nsens++) {
  //      Mechanics::Sensor* sensor = _refDevice->getSensor(nsens);
  //      sensor->clearNoisyPixels();
  //      TH2D* occupancyPlot = occupancy.getHitOcc(nsens);
  //      for (unsigned int x = 0; x < sensor->getNumX(); x++)
  //      {
  //        for (unsigned int y = 0; y < sensor->getNumY(); y++)
  //        {
  //          double pixelOcc = occupancyPlot->GetBinContent(x + 1, y + 1) /
  //              (double)occupancy.totalHitOccupancy;
  //          if (pixelOcc > cutoff)
  //          {
  //            sensor->addNoisyPixel(x, y);
  //            noisyPixels++;
  //          }
  //        }
  //      }
  //    }
  
  //cout << "Masking " << 100 * noisyPixels / (double)(_refDevice->getNumPixels())
  //       << "% of pixels (ammount: " << noisyPixels << ")" << endl;
  
  //  cout << "Accept? (y/n) ";
  //  char accept = 0;
  //  cin >> accept;
  //  if (accept == 'y') break;
  //}
  

  //TFile *fout = new TFile("occupancies.root","RECREATE");
  //  TH1D* hist = occupancy.getOccDistribution();
  //  hist->Write();
  //  for(unsigned int i=0; i<_refDevice->getNumSensors(); i++)
  //    occupancy.getHitOcc(i)->Write();
  //  fout->Close();
      
  unsigned int noisyPixels = 0;  
  for(unsigned int nsens=0; nsens<_refDevice->getNumSensors(); nsens++){
    Mechanics::Sensor* sensor = _refDevice->getSensor(nsens);
    sensor->clearNoisyPixels();
    
    TH2D* occupancyPlot = occupancy.getHitOcc(nsens);
    
    std::vector<double> occupancies;
    unsigned int numEmpty = 0;
    
    //If no values have been defined: use the whole sensor
    if (_bottomLimitX<0 || _bottomLimitX>sensor->getNumX()) _bottomLimitX=0;
    if (_upperLimitX<0 || _upperLimitX>sensor->getNumX()) _upperLimitX=sensor->getNumX();
    if (_bottomLimitY<0 || _bottomLimitY>sensor->getNumY()) _bottomLimitY=0;
    if (_upperLimitY<0 || _upperLimitY>sensor->getNumY()) _upperLimitY=sensor->getNumY();
    
    for(unsigned int x = _bottomLimitX; x<_upperLimitX; x++){
      for(unsigned int y = _bottomLimitY; y<_upperLimitY; y++){
	double pixelOcc = occupancyPlot->GetBinContent(x+1,y+1) /
	  (double)occupancy.totalHitOccupancy;
	
	    occupancies.push_back(pixelOcc);
	    if(pixelOcc==0) numEmpty++;
	    
	    if (_maxOccupancy && pixelOcc > _maxOccupancy){
	      sensor->addNoisyPixel(x,y);
	      noisyPixels++;
	    }
      }
    }
    
    // If a max occupancy is specified, don't try to use max factor
    if (_maxOccupancy) continue;

    // sort vector. Dead pixels (occupancy=0) will be placed at front.
    std::sort( occupancies.begin(), occupancies.end() );

    const unsigned int occHalf = (occupancies.size() - numEmpty) / 2;
    double average = 0;
    for(unsigned int i=numEmpty; i<occHalf; i++)
      average += occupancies.at(i);
    if (occHalf) average /= (double)occHalf;

    //    cout << endl  << "Plane " << nsens << endl;
    //    cout << "  - occupancies.size() = "<< occupancies.size() << endl;
    //    cout << "  - numEmpty = "<< numEmpty << endl;
    //    cout << "  - average  = "<< average << endl;
    
    for(unsigned int x=0; x<sensor->getNumX(); x++){
	for(unsigned int y=0; y<sensor->getNumY(); y++){
	  double pixelOcc = occupancyPlot->GetBinContent(x+1,y+1) /
	    (double)occupancy.totalHitOccupancy;
	  
	  if(pixelOcc > _maxFactor*average){
	    sensor->addNoisyPixel(x,y);
	    noisyPixels++;
	  }
	}
    }
    
    occupancies.clear();

  } // end loop in planes

  cout << "Masking " << 100 * noisyPixels / (double)(_refDevice->getNumPixels())
       << "% of pixels (ammount: " << noisyPixels << ")" << endl;
  
  _refDevice->print();  
  _refDevice->getNoiseMask()->writeMask();
}


void Loopers::NoiseScan::setMaxFactor(double maxFactor) { _maxFactor = maxFactor; }
void Loopers::NoiseScan::setMaxOccupancy(double maxOccupancy) { _maxOccupancy = maxOccupancy; }
void Loopers::NoiseScan::setBottomLimitX(unsigned int bottomLimitX) { _bottomLimitX = bottomLimitX; }
void Loopers::NoiseScan::setUpperLimitX(unsigned int upperLimitX) { _upperLimitX = upperLimitX; }
void Loopers::NoiseScan::setBottomLimitY(unsigned int bottomLimitY) { _bottomLimitY = bottomLimitY; }
void Loopers::NoiseScan::setUpperLimitY(unsigned int upperLimitY) { _upperLimitY = upperLimitY; }

void Loopers::NoiseScan::print(){
  cout << "## [NoiseScan::print]" << endl;
  cout << "  - limitsX   : [" << _bottomLimitX << " , " << _upperLimitX << "]" << endl;
  cout << "  - limitsY   : [" << _bottomLimitY << " , " << _upperLimitY << "]" << endl;
  cout << "  - maxFactor : "  << _maxFactor << endl;
  cout << "  - maxOccu   : "  << _maxOccupancy << endl;
  cout << endl;
}
