#include "noisescan.h"

#include <cassert>
#include <vector>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>
#include <ctime>

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
using std::string;

//=========================================================
Loopers::NoiseScanConfig::NoiseScanConfig() :
  _maxFactor(10),
  _maxOccupancy(0),
  _bottomLimitX(-1),
  _upperLimitX(-1),
  _bottomLimitY(-1),
  _upperLimitY(-1)
{
  _vruns.reserve(100);
  _vruns.clear();
}

//=========================================================
Loopers::NoiseScanConfig::NoiseScanConfig(const std::vector<int> &runs) :
  _maxFactor(10),
  _maxOccupancy(0),
  _bottomLimitX(-1),
  _upperLimitX(-1),
  _bottomLimitY(-1),
  _upperLimitY(-1),
  _vruns(runs)
{ }

//=========================================================
Loopers::NoiseScanConfig::NoiseScanConfig(const Loopers::NoiseScanConfig &nc) {
  if(this!=&nc){
    (*this)=nc;
  }
}

//=========================================================
Loopers::NoiseScanConfig &Loopers::NoiseScanConfig::operator=(const Loopers::NoiseScanConfig &nc){
  if(this!=&nc){
    _maxFactor    = nc._maxFactor;
    _maxOccupancy = nc._maxOccupancy;
    _bottomLimitX = nc._bottomLimitX;
    _upperLimitX  = nc._upperLimitX;
    _bottomLimitY = nc._bottomLimitY;
    _upperLimitY  = nc._upperLimitY;
    _vruns        = nc._vruns;
  }
  return *this;
}

//=========================================================
const std::string Loopers::NoiseScanConfig::print() const {
  std::ostringstream out;
  std::time_t now = time(0);
  out << endl;
  out << "#DO NOT REMOVE THE LINES BELOW; THEY CONTAIN INFO "
      << "ABOUT HOW THIS FILE WAS CREATED" << endl;
  out << "#Metadata created on " << ctime(&now);
  out << "#[Noise Scan]" << endl;
  out << "#  runs : ";
  for(std::vector<int>::const_iterator cit=_vruns.begin();
      cit!=_vruns.end(); ++cit){
    out << (*cit);
    if( cit < _vruns.end()-1 ) out << ", ";      
  }
  out << endl;  
  out << "#  max factor : " << _maxFactor << endl;
  out << "#  max occupancy : " << _maxOccupancy << endl;
  out << "#  bottom x : " << _bottomLimitX << endl;
  out << "#  upper x : " << _upperLimitX << endl;
  out << "#  bottom y : " << _bottomLimitY << endl;
  out << "#  upper y : " << _upperLimitY << endl;
  out << "#[End Noise Scan]" << endl;
  return out.str();
}

//=========================================================
Loopers::NoiseScan::NoiseScan(Mechanics::Device* refDevice,
			      Storage::StorageIO* refInput,
			      ULong64_t startEvent,
			      ULong64_t numEvents,
			      unsigned int eventSkip) :
  Looper(refInput, 0, startEvent, numEvents, eventSkip),
  _refDevice(refDevice),
  _config(0),
  _printLevel(0)
{
  assert(refInput && refDevice &&
	 "Looper: initialized with null object(s)");
  
  assert(refInput->getNumPlanes() == refDevice->getNumSensors() &&
         "Loopers: number of planes / sensors mis-match");
  
  _config = new NoiseScanConfig(refInput->getRuns());
}

//=========================================================
Loopers::NoiseScan::~NoiseScan(){
  delete _config;
}

//=========================================================
void Loopers::NoiseScan::loop(){
  
  /* create Occupancy analyzer with null TDirectory
     (histograms will not be written to any output file) */
  Analyzers::Occupancy occupancy(_refDevice, NULL);

  int totEvt=0;
  for (ULong64_t nevent=_startEvent; nevent<=_endEvent; nevent++, totEvt++){
    Storage::Event* refEvent = _refStorage->readEvent(nevent);    
    occupancy.processEvent(refEvent);    
    progressBar(nevent);    
    delete refEvent;
  }
  cout << "\nnevents = "<< totEvt << endl;
  occupancy.postProcessing();

  // output file
  TFile *fout = new TFile("occupancies.root","RECREATE");
  for(unsigned int i=0; i<_refDevice->getNumSensors(); i++){
    occupancy.getHitOcc(i)->Write();
    occupancy.getHitOccDist(i)->Write();
  } 
  
  // loop in planes and determine noisy pixels  
  for(unsigned int nsens=0; nsens<_refDevice->getNumSensors(); nsens++){
    Mechanics::Sensor* sensor = _refDevice->getSensor(nsens);
    sensor->clearNoisyPixels();
    
    /** define boundary region to look for noisy pixels: if no values have been defined,
	or if out-of bounds, use the whole sensor region */
    if( (_config->getBottomLimitX() < 0) || (_config->getBottomLimitX() > sensor->getNumX()) )
      _config->setBottomLimitX(0);
    
    if( (_config->getUpperLimitX() < 0) || (_config->getUpperLimitX() > sensor->getNumX()) )
      _config->setUpperLimitX(sensor->getNumX());
    
    if( (_config->getBottomLimitY() < 0) || (_config->getBottomLimitY() > sensor->getNumY()) )
      _config->setBottomLimitY(0);
    
    if( (_config->getUpperLimitY() < 0) || (_config->getUpperLimitY() > sensor->getNumY()) )
      _config->setUpperLimitY(sensor->getNumY());
    
    std::vector<double> occupancies;
    unsigned int noisyPixels=0;
    unsigned int numEmpty=0;
    
    double totOcc = (double)occupancy.getTotalHitOccupancy(nsens);
    //double totOcc = 1;
    
    TH2D* occupancyPlot = occupancy.getHitOcc(nsens);    
    for(unsigned int x=_config->getBottomLimitX(); x<_config->getUpperLimitX(); x++){
      for(unsigned int y=_config->getBottomLimitY(); y<_config->getUpperLimitY(); y++){
	
	// single pixel occupancy
	double pixelOcc = totOcc!=0 ? occupancyPlot->GetBinContent(x+1,y+1) / totOcc : 0;
	
	occupancies.push_back(pixelOcc);
	if(pixelOcc==0)
	  numEmpty++;
	
	if(_config->getMaxOccupancy() && (pixelOcc > _config->getMaxOccupancy()) ){
	  sensor->addNoisyPixel(x,y);
	  noisyPixels++;
	}
      }
    }
    
    // If a max occupancy is specified, don't try to use max factor
    if( _config->getMaxOccupancy() ) continue;
    
    // sort vector. Dead pixels (occupancy=0) will be placed at front.
    std::sort(occupancies.begin(),occupancies.end());
    
    const unsigned int occHalf = (occupancies.size() - numEmpty) / 2;
    double average = 0;
    for(unsigned int i=numEmpty; i<occHalf; i++)
      average += occupancies.at(i);
    if(occHalf) average /= (double)occHalf;
    
    double maxOcc = (_config->getMaxFactor()) * average;

    for(unsigned int x=0; x<sensor->getNumX(); x++){
      for(unsigned int y=0; y<sensor->getNumY(); y++){
	double pixelOcc = totOcc!=0 ? occupancyPlot->GetBinContent(x+1,y+1) / totOcc : 0;
	if( pixelOcc > maxOcc ) {
	  sensor->addNoisyPixel(x,y);
	  noisyPixels++;
	}
      }
    }

    if( _printLevel > 1 ){
      cout << "\nAnalyzing sensor " << nsens << endl;
      cout << "  - totOcc = "<< totOcc << endl;
      cout << "  - occHalf = "<< occHalf << endl;
      cout << "  - numEmpty = "<< numEmpty << endl;
      cout << "  - average  = "<< average << endl;
      cout << "  - maxOcc  = "<< maxOcc << endl;
      cout << "  - masking " << 100*noisyPixels / (double)(sensor->getNumPixels())
	   << "% of pixels (ammount: " << noisyPixels << ")" << endl;
    }

    occupancies.clear();    
    
  } // end loop in planes

  fout->Close();
    
  // show masked pixels for the different planes
  if( _printLevel > 0 ){
    cout << "\nNoiseScan summary" << endl;
    for(unsigned int nsens=0; nsens<_refDevice->getNumSensors(); nsens++){
      Mechanics::Sensor* sensor = _refDevice->getSensor(nsens);
      cout << " - Sensor '" << sensor->getName() << "' has "
	   << sensor->getNumNoisyPixels() << " noisy pixels ("
	   << 100*sensor->getNumNoisyPixels() / (double)(sensor->getNumPixels())
	   << " %)";      
      if( sensor->getNumNoisyPixels() == 0 ) { cout << endl; continue; }
      if( _printLevel > 2 ){
	cout << " These are (col,row):" << endl;
	for(unsigned int i=0; i<sensor->getNumX(); i++){
	  for(unsigned int j=0; j<sensor->getNumY(); j++){
	    if( sensor->isPixelNoisy(i,j) ){
	      cout << " (" << std::setw(2) << i << " , " << std::setw(3) << j << ") " << endl;
	    }
	  }
	}
      }
      else
	cout << endl;
    }
  }

  // write mask, including metadata from this looper
  _refDevice->getNoiseMask()->writeMask(_config);
}

//=========================================================
void Loopers::NoiseScan::print(){
  cout << "[NoiseScan::print]" << endl;
  if( _config->getMaxOccupancy() ) cout << " - maxOccu   : "  << _config->getMaxOccupancy() << endl;
  else                             cout << " - maxFactor : "  << _config->getMaxFactor() << endl;
  cout << " - limitsX   : (" << _config->getBottomLimitX() << " , " << _config->getUpperLimitX() << ")" << endl;
  cout << " - limitsY   : (" << _config->getBottomLimitY() << " , " << _config->getUpperLimitY() << ")" << endl;
  cout << " - printLevel: " << _printLevel << endl;
  cout << endl;
}

