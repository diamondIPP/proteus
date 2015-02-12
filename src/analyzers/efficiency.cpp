#include "efficiency.h"

#include <cassert>
#include <sstream>
#include <math.h>
#include <vector>
#include <float.h>

#include <TDirectory.h>
#include <TH2D.h>
#include <TH1D.h>
#include <TError.h>
#include <TMath.h>

// Access to the device being analyzed and its sensors
#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
// Access to the data stored in the event
#include "../storage/hit.h"
#include "../storage/cluster.h"
#include "../storage/plane.h"
#include "../storage/track.h"
#include "../storage/event.h"
// Some generic processors to calcualte typical event related things
#include "../processors/processors.h"
// This header defines all the cuts
#include "cuts.h"

#ifndef VERBOSE
#define VERBOSE 1
#endif

using std::cout;
using std::endl;

//=========================================================
Analyzers::Efficiency::Efficiency(const Mechanics::Device* refDevice,
				  const Mechanics::Device* dutDevice,
				  TDirectory* dir,
				  const char* suffix,
				  int relativeToSensor,
				  int pix_x_min,
				  int pix_x_max,
				  int pix_y_min,
				  int pix_y_max,
				  int min_entries_lvl1,
				  int min_entries_lvl1_matchedTracks,
				  unsigned int rebinX,
				  unsigned int rebinY,
				  unsigned int pixBinsX,
				  unsigned int pixBinsY) :
  DualAnalyzer(refDevice, dutDevice, dir, suffix),
  _relativeToSensor(relativeToSensor),
  _pix_x_min(pix_x_min),
  _pix_x_max(pix_x_max),
  _pix_y_min(pix_y_min),
  _pix_y_max(pix_y_max),
  _min_entries_lvl1(min_entries_lvl1),
  _min_entries_lvl1_matchedTracks(min_entries_lvl1_matchedTracks)
{
  assert(refDevice && dutDevice && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* plotDir = makeGetDirectory("Efficiency");

  std::stringstream name; // Build name strings for each histo
  std::stringstream title; // Build title strings for each histo
  std::stringstream Xaxis;

  if(_relativeToSensor >= (int)_dutDevice->getNumSensors())
    throw "Efficiency: relative sensor exceeds range";
  
  // Generate a histogram for each sensor in the device
  for (unsigned int nsens=0; nsens<_dutDevice->getNumSensors(); nsens++){
    Mechanics::Sensor* sensor = _dutDevice->getSensor(nsens);
    
    unsigned int nx = sensor->getNumX() / rebinX;
    unsigned int ny = sensor->getNumY() / rebinY;
    if (nx < 1) nx = 1;
    if (ny < 1) ny = 1;
    const double lowX = sensor->getOffX() - sensor->getSensitiveX() / 2.0;
    const double uppX = sensor->getOffX() + sensor->getSensitiveX() / 2.0;
    const double lowY = sensor->getOffY() - sensor->getSensitiveY() / 2.0;
    const double uppY = sensor->getOffY() + sensor->getSensitiveY() / 2.0;

    // TODO: change to pixel space

    // Efficiency map initialization
    name.str(""); title.str("");
    name << sensor->getDevice()->getName() << sensor->getName() << "Map" << _nameSuffix;
    title << sensor->getDevice()->getName() << " - " << sensor->getName()
	  << " [Efficiency Map]"
          << ";X position" << " [" << _dutDevice->getSpaceUnit() << "]"
          << ";Y position" << " [" << _dutDevice->getSpaceUnit() << "]"
          << ";Efficiency";
    TEfficiency* map = new TEfficiency(name.str().c_str(), title.str().c_str(),
                                       nx, lowX, uppX,
                                       ny, lowY, uppY);
    map->SetDirectory(plotDir);
    map->SetStatisticOption(TEfficiency::kFWilson);
    _efficiencyMap.push_back(map);

    // Pixel grouped efficieny initialization
    name.str(""); title.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
         << "GroupedDistribution" << _nameSuffix;
    title << sensor->getDevice()->getName() << " - " << sensor->getName()
          << " [Grouped Efficiency]";
    TH1D* pixels = new TH1D(name.str().c_str(), title.str().c_str(),
                            20, 0, 1.001);
    pixels->GetXaxis()->SetTitle("Pixel group efficiency");
    pixels->GetYaxis()->SetTitle("Pixel groups / 0.05");
    pixels->SetDirectory(plotDir);
    _efficiencyDistribution.push_back(pixels);
    
    // Track matched position (X)
    name.str(""); title.str(""); Xaxis.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
         << "MatchedTracksPositionX" << _nameSuffix;
    title << sensor->getDevice()->getName() << " " << sensor->getName()
          << " [Matched tracks : Position-X]";
    TH1D* matchedPositionX = new TH1D(name.str().c_str(), title.str().c_str(),
				      sensor->getNumX()*1.5*3, 1.5*lowX, 1.5*uppX);
    Xaxis << "X-position" << " [" << _dutDevice->getSpaceUnit() << "]";
    matchedPositionX->GetXaxis()->SetTitle(Xaxis.str().c_str());
    matchedPositionX->SetDirectory(plotDir);
    _matchPositionX.push_back(matchedPositionX);
    
    // Track matched position (Y)
    name.str(""); title.str(""); Xaxis.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
         << "MatchedTracksPositionY" << _nameSuffix;
    title << sensor->getDevice()->getName() << " - " << sensor->getName()
          << " [Matched tracks : Position-Y]"; 
    TH1D* matchedPositionY = new TH1D(name.str().c_str(), title.str().c_str(),
				      sensor->getNumY()*1.5*3, 1.5*lowY, 1.5*uppY);
    Xaxis << "Y-position" << " [" << _dutDevice->getSpaceUnit() << "]";
    matchedPositionY->GetXaxis()->SetTitle(Xaxis.str().c_str());
    matchedPositionY->SetDirectory(plotDir);
    _matchPositionY.push_back(matchedPositionY);

    //-----------------------------------------------------------
    name.str(""); title.str(""); Xaxis.str("");
    name << sensor->getDevice()->getName() << sensor->getName() << "MatchedTiming" << _nameSuffix;
    title << sensor->getDevice()->getName() << " - " << sensor->getName()
	  << " [Matched tracks : Timing]";
    TH1D* matchTiming = new TH1D(name.str().c_str(), title.str().c_str(),
				 16, 0, 16);
    matchTiming->GetXaxis()->SetTitle("Timing [BC]");
    matchTiming->GetYaxis()->SetTitle("# hits");
    matchTiming->SetDirectory(plotDir);
    _matchTime.push_back(matchTiming);

    //-----------------------------------------------------------
    name.str(""); title.str(""); Xaxis.str("");
    name << sensor->getDevice()->getName() << sensor->getName() << "Matched_ToT" << _nameSuffix;
    title << sensor->getDevice()->getName() << " - " << sensor->getName() << " [ToT distribution]";
    TH1D* matchToT = new TH1D(name.str().c_str(), title.str().c_str(),
			      18, 0,18);
    matchToT->GetXaxis()->SetTitle("ToT [BC]");
    matchToT->SetDirectory(plotDir);
    _matchToT.push_back(matchToT);

    //-----------------------------------------------------------
    name.str(""); title.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
	 << "lvl1_vs_ToT" << _nameSuffix;
    title << sensor->getDevice()->getName() << " - " << sensor->getName()
	  << " [Timing vs ToT]";
    TH2D* lvl1vsToT = new TH2D(name.str().c_str(), title.str().c_str(),
			       16, 0, 16, // LVL1
			       18, 0, 18); // ToT
    lvl1vsToT->GetXaxis()->SetTitle("LVL1 [BC]");
    lvl1vsToT->GetYaxis()->SetTitle("ToT [BC]");
    lvl1vsToT->SetDirectory(plotDir);
    _lvl1vsToT.push_back(lvl1vsToT);

    //-----------------------------------------------------------
    name.str(""); title.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
	 << "InPixelTiming" << _nameSuffix;
    title << sensor->getDevice()->getName() << " - " << sensor->getName()
	  << " [InPixel Timing Map]";
    TH2D* inTimingMap = new TH2D(name.str().c_str(), title.str().c_str(),
				 pixBinsX, 0, sensor->getPitchX(),
                                 pixBinsY, 0, sensor->getPitchY());
    inTimingMap->SetDirectory(plotDir);
    _inPixelTiming.push_back(inTimingMap);

    //==================================================
    TDirectory* plotDir1 = makeGetDirectory("Efficiency/InpixelLvl1");

    std::vector<TH2D*> tmp;    
    for(int bc=0; bc<16; bc++){
      name.str(""); title.str("");
      name << sensor->getDevice()->getName() << sensor->getName()
	   << Form("InPixelTiming_bc_%d",bc) << _nameSuffix;
      title << sensor->getDevice()->getName() << " " << sensor->getName()
	    << Form(" InPixel Timing Map for lvl= %d bc", bc);
      TH2D* inTimingMap1 = new TH2D(name.str().c_str(), title.str().c_str(),
				    pixBinsX, 0, sensor->getPitchX(),
				    pixBinsY, 0, sensor->getPitchY());
      inTimingMap1->SetDirectory(plotDir1);     
      tmp.push_back(inTimingMap1);
    }
    _inPixelTimingLVL1.push_back(tmp);
    tmp.clear();   

    // InPixel efficiency : all LVL1 
    name.str(""); title.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
         << "total" << _nameSuffix;
    title << sensor->getDevice()->getName() << " " << sensor->getName()
          << " InPixel Efficiency Map for lvl=  bc";
    TH2D* inPixel_LVL1_total = new TH2D(name.str().c_str(), title.str().c_str(),
					pixBinsX, 0, sensor->getPitchX(),
					pixBinsY, 0, sensor->getPitchY());
    inPixel_LVL1_total->SetDirectory(plotDir);
    _inPixelEfficiencyLVL1_total.push_back(inPixel_LVL1_total);

    //-----------------------------------------------------------
    std::vector<TH2D*> tmp2;
    for(int bc=0; bc<16; bc++){
      name.str(""); title.str("");
      name << sensor->getDevice()->getName() << sensor->getName()
	   << Form("InPixelEfficiencypa_bc_%d",bc) << _nameSuffix;
      title << sensor->getDevice()->getName() << " " << sensor->getName()
	    << Form(" In Pixel Efficiency Map for lvl= %d bc",bc);      
      TH2D* inPixel_LVL1_passed = new TH2D(name.str().c_str(), title.str().c_str(),
						    pixBinsX, 0, sensor->getPitchX(),
						    pixBinsY, 0, sensor->getPitchY());
      inPixel_LVL1_passed->SetDirectory(plotDir);
      tmp2.push_back(inPixel_LVL1_passed);
    }
    _inPixelEfficiencyLVL1_passed.push_back(tmp2);
    tmp2.clear();
    
    //-----------------------------------------------------------
    name.str(""); title.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
         << "InPixelToT" << _nameSuffix;
    title << sensor->getDevice()->getName() << " " << sensor->getName()
          << " In Pixel ToT Map";
    TH2D* inToTMap = new TH2D(name.str().c_str(), title.str().c_str(),
			      pixBinsX, 0, sensor->getPitchX(),
			      pixBinsY, 0, sensor->getPitchY());
    inToTMap->SetDirectory(plotDir);
    _inPixelToT.push_back(inToTMap);
    
    //-----------------------------------------------------------
    name.str(""); title.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
	 << "InPixelTimingCnt" << _nameSuffix;
    title << sensor->getDevice()->getName() << " - " << sensor->getName()
          << " [InPixel Timing Map counting]";
    TH2D* count = new TH2D(name.str().c_str(), title.str().c_str(),
				 pixBinsX, 0, sensor->getPitchX(),
                                 pixBinsY, 0, sensor->getPitchY());
    count->SetDirectory(0);
    _inPixelCounting.push_back(count);

    // Track matching information
    name.str(""); title.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
         << "MatchedTracks" << _nameSuffix;
    title << sensor->getDevice()->getName() << " " << sensor->getName()
          << " [Matched Tracks]";
    TH1D* matched = new TH1D(name.str().c_str(), title.str().c_str(),
                             2, -0.5, 1.5);
    matched->GetXaxis()->SetTitle("0 : unmatched tracks | 1 : matched tracks");
    matched->SetDirectory(plotDir);
    _matchedTracks.push_back(matched);

    //-----------------------------------------------------------
    name.str(""); title.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
         <<  "InPixelEfficiency" << _nameSuffix;
    title << sensor->getDevice()->getName() << " " << sensor->getName()
          << " In Pixel Efficiency"
          << ";X position [" << _dutDevice->getSpaceUnit() << "]"
          << ";Y position [" << _dutDevice->getSpaceUnit() << "]"
          << "";
    TEfficiency* inPixelEfficiency = new TEfficiency(name.str().c_str(), title.str().c_str(),
                                                     pixBinsX, 0, sensor->getPitchX(),
                                                     pixBinsY, 0, sensor->getPitchY());
    inPixelEfficiency->SetDirectory(plotDir);
    _inPixelEfficiency.push_back(inPixelEfficiency);
    
    //-----------------------------------------------------------
    if (_refDevice->getTimeEnd() > _refDevice->getTimeStart()) // If not used, they are both == 0
    {
      // Prevent aliasing
      const unsigned int nTimeBins = 100;
      const ULong64_t timeSpan = _refDevice->getTimeEnd() - _refDevice->getTimeStart() + 1;
      const ULong64_t startTime = _refDevice->getTimeStart();
      const ULong64_t endTime = timeSpan - (timeSpan % nTimeBins) + startTime;

      name.str(""); title.str("");
      name << sensor->getDevice()->getName() << sensor->getName()
           << "EfficiencyVsTime" << _nameSuffix;
      title << sensor->getDevice()->getName() << " " << sensor->getName()
            << " Efficiency Vs. Time"
            << ";Time [" << _refDevice->getTimeUnit() << "]"
            << ";Average efficiency";
      TEfficiency* efficiencyTime = new TEfficiency(name.str().c_str(), title.str().c_str(),
                                                    nTimeBins,
                                                    _refDevice->tsToTime(startTime),
                                                    _refDevice->tsToTime(endTime + 1));
      efficiencyTime->SetDirectory(plotDir);
      efficiencyTime->SetStatisticOption(TEfficiency::kFWilson);
      _efficiencyTime.push_back(efficiencyTime);
    }
  }
}

//=========================================================
void Analyzers::Efficiency::processEvent(const Storage::Event* refEvent,
					 const Storage::Event* dutEvent){

  assert(refEvent && dutEvent && "Analyzer: can't process null events");
  
  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(refEvent, dutEvent);
  
  // Check if the event passes the cuts
  for(unsigned int ncut=0; ncut<_numEventCuts; ncut++)
    if (!_eventCuts.at(ncut)->check(refEvent)) return;
  
  for (unsigned int ntrack=0; ntrack<refEvent->getNumTracks(); ntrack++){
    Storage::Track* track = refEvent->getTrack(ntrack);

    // Check if the track passes the cuts
    bool pass = true;
    for (unsigned int ncut=0; ncut<_numTrackCuts; ncut++)
      if (!_trackCuts.at(ncut)->check(track)) { pass = false; break; }
    if (!pass) continue;
    
    // Make a list of the planes with their matched cluster
    std::vector<Storage::Cluster*> matches;
    for(unsigned int nplane=0; nplane<dutEvent->getNumPlanes(); nplane++)
      matches.push_back(0); // No matches
    
    // Get the matches from the track
    for(unsigned int nmatch=0; nmatch<track->getNumMatchedClusters(); nmatch++){
      Storage::Cluster* cluster = track->getMatchedCluster(nmatch);
      
      // Check if this cluster passes the cuts
      //bool pass = true; // <--------- [SGS] WHY COMMENTED ????
      for (unsigned int ncut=0; ncut<_numClusterCuts; ncut++){
        if (!_clusterCuts.at(ncut)->check(cluster)) {
	  pass = false;
	  break; 
	}
	
	//Bilbao@cern.ch:Cut for cluster size (also in DUTresiduals to affect all eff.root results)
        /*if(cluster->getNumHits()==1){
	  std::cout << cluster->getNumHits() << std::endl;
            pass = false;
            break;}*/

	pass = true;
	matches.at(cluster->getPlane()->getPlaneNum()) = cluster;
      }
    }

    assert(matches.size() == _dutDevice->getNumSensors() &&
           _relativeToSensor < (int)_dutDevice->getNumSensors() &&
           "Efficiency: matches has the wrong size");

    // If asking for relative efficiency to a plane, look for a match in that one first     
    if (_relativeToSensor >= 0 && !matches.at(_relativeToSensor)) continue;

    for (unsigned int nsensor=0; nsensor<_dutDevice->getNumSensors(); nsensor++){
      Mechanics::Sensor* sensor = _dutDevice->getSensor(nsensor);

      double tx=0, ty=0, tz=0;
      Processors::trackSensorIntercept(track, sensor, tx, ty, tz);

      double px=0, py=0;
      sensor->spaceToPixel(tx, ty, tz, px, py);

      const Storage::Cluster* match = matches.at(nsensor);

      // Check if the intercepted pixel is hit
//      bool trackMatchesPixel = false;
//      unsigned int pixelX = px;
//      unsigned int pixelY = py;
//      if (match)
//      {
//        for (unsigned int nhit = 0; nhit < match->getNumHits(); nhit++)
//        {
//          const Storage::Hit* hit = match->getHit(nhit);
//          if (hit->getPixX() == pixelX && hit->getPixY() == pixelY)
//          {
//            trackMatchesPixel = true;
//            break;
//          }
//        }
//      }

      // Get the location within the pixel where the track interception
      const double trackPosX = px - (int)px;
      const double trackPosY = py - (int)py;

      // TO BE PROPERLY SET!!!!!
      //      if(px >1 & px<12 & py>81 & py<94){
      //      if(px >1 && px<6 && py>81 && py<84){
      //  if(px>1 && px<5  && py>85 && py<87   ){
      if( (px > _pix_x_min) && (px < _pix_x_max) && (py > _pix_y_min) && (py < _pix_y_max) ){

	_inPixelEfficiency.at(nsensor)->Fill((bool)match,
					     trackPosX * sensor->getPitchX(),
					     trackPosY * sensor->getPitchY());

	_inPixelEfficiencyLVL1_total.at(nsensor)->Fill(trackPosX * sensor->getPitchX(),
						       trackPosY * sensor->getPitchY());
	
	if(match){
	  for(unsigned int nhit=0; nhit<match->getNumHits(); nhit++){
	    const Storage::Hit *hit = match->getHit(nhit);

	    _inPixelTiming.at(nsensor)->Fill(trackPosX * sensor->getPitchX(),
					     trackPosY * sensor->getPitchY(),
					     hit->getTiming());
	    
	    _inPixelToT.at(nsensor)->Fill(trackPosX * sensor->getPitchX(),
					  trackPosY * sensor->getPitchY(),
					  hit->getValue());
	    
	    _inPixelCounting.at(nsensor)->Fill(trackPosX * sensor->getPitchX(),
					       trackPosY * sensor->getPitchY());


	    int lvl1 = (int)hit->getTiming();
	    
	    (_inPixelEfficiencyLVL1_passed.at(nsensor))[lvl1]->Fill(trackPosX * sensor->getPitchX(),
								    trackPosY * sensor->getPitchY());
	    
	    (_inPixelTimingLVL1.at(nsensor))[lvl1]->Fill(trackPosX * sensor->getPitchX(),
							 trackPosY * sensor->getPitchY());
	  }
	}
      }

      if (match)
      {
        //_efficiencyMap.at(nsensor)->Fill(true, tx, ty);

	for (unsigned int nhit=0; nhit<match->getNumHits(); nhit++){
	  const Storage::Hit *hit = match->getHit(nhit);
	  _matchTime.at(nsensor)->Fill(hit->getTiming());
	  _matchToT.at(nsensor)->Fill(hit->getValue());
	  _lvl1vsToT.at(nsensor)->Fill(hit->getTiming(),hit->getValue());
        } 
	
	//bilbao@cern.ch / reina.camacho@cern.ch: Filling the efficiency histogram with the cluster position instead of the track position
	_efficiencyMap.at(nsensor)->Fill(true, match->getPosX(), match->getPosY());
        _matchedTracks.at(nsensor)->Fill(1);
	_matchPositionX.at(nsensor)->Fill(tx);
	_matchPositionY.at(nsensor)->Fill(ty);
        if (_efficiencyTime.size())
          _efficiencyTime.at(nsensor)->Fill(true, _refDevice->tsToTime(refEvent->getTimeStamp()));
      }
      else
      {
        _efficiencyMap.at(nsensor)->Fill(false, tx, ty);
        _matchedTracks.at(nsensor)->Fill(0);
        if (_efficiencyTime.size())
          _efficiencyTime.at(nsensor)->Fill(false, _refDevice->tsToTime(refEvent->getTimeStamp()));
      }
    }
  }
}



//=========================================================
void Analyzers::Efficiency::postProcessing() {
  if (_postProcessed) return;
  
  for (unsigned int nsensor=0; nsensor<_dutDevice->getNumSensors(); nsensor++){

    TEfficiency* efficiency = _efficiencyMap.at(nsensor);
    const TH1* values = efficiency->GetTotalHistogram();
    TH1D* distribution = _efficiencyDistribution.at(nsensor);
    
    // InPixel timing/ToT plots and renormalization
    TH2D* inTimingMap =  _inPixelTiming.at(nsensor);
    TH2D* inToTMap =  _inPixelToT.at(nsensor);
    TH2D* count = _inPixelCounting.at(nsensor);

    double val=0;
    for(int x=1; x<=inTimingMap->GetNbinsX(); x++){
      for(int y=1; y<=inTimingMap->GetNbinsY(); y++){
	
	val = count->GetBinContent(x,y) != 0 ? inTimingMap->GetBinContent(x,y) / count->GetBinContent(x,y) : 0;
	// [SGS] why overwritting the original histo instead of filling a new one ???
	inTimingMap->SetBinContent(x,y,val);
	
	val = count->GetBinContent(x,y) != 0 ? inToTMap->GetBinContent(x,y) / count->GetBinContent(x,y) : 0;
	// [SGS] why overwritting the original histo instead of filling a new one ???
	inToTMap->SetBinContent(x,y,val);

	// loop in the 16 LVL1 bunch-crossings
	for(int bc=0; bc<16; bc++){
	  TH2D *hist = _inPixelTimingLVL1.at(nsensor)[bc];

	  if( hist->GetBinContent(x,y) > _min_entries_lvl1 ){	  
	    val = count->GetBinContent(x,y) != 0 ? hist->GetBinContent(x,y) / count->GetBinContent(x,y) : 0;
	    // [SGS] why overwritting the original histo instead of filling a new one ???
	    hist->SetBinContent(x,y,val);
	  }

	  // for matched tracks
	  TH2D *hpass = _inPixelEfficiencyLVL1_passed.at(nsensor)[bc];	  
	  TH2D *htot = _inPixelEfficiencyLVL1_total[nsensor];	  
	  if( hpass->GetBinContent(x,y) > _min_entries_lvl1_matchedTracks ){
	    val = htot->GetBinContent(x,y) != 0 ?  hpass->GetBinContent(x,y) / htot->GetBinContent(x,y) : 0;
	    // [SGS] why overwritting the original histo instead of filling a new one ???
	    hpass->SetBinContent(x,y,val);
	  }
	} // end loop in BC	

      }
    }
    
    // Loop over all pixel groups
    for(int binx=1; binx<=values->GetNbinsX(); ++binx){
      for (int biny=1; biny<=values->GetNbinsY(); ++biny){
        const Int_t bin = values->GetBin(binx,biny);
        if (values->GetBinContent(binx, biny) < 1) continue;
        const double value = efficiency->GetEfficiency(bin);
        const double sigmaLow = efficiency->GetEfficiencyErrorLow(bin);
        const double sigmaHigh = efficiency->GetEfficiencyErrorUp(bin);
        // Find the probability of this pixel group being found in all bins of the distribution
        double normalization = 0;
	
        for (int distBin=1; distBin<=distribution->GetNbinsX(); distBin++){
          const double evaluate = distribution->GetBinCenter(distBin);
          const double sigma = (evaluate < value) ? sigmaLow : sigmaHigh;
          const double weight = TMath::Gaus(evaluate, value, sigma);
          normalization += weight;
        }
	
        for(int distBin=1; distBin<=distribution->GetNbinsX(); distBin++){
          const double evaluate = distribution->GetBinCenter(distBin);
          const double sigma = (evaluate < value) ? sigmaLow : sigmaHigh;
          const double weight = TMath::Gaus(evaluate, value, sigma);
          distribution->Fill(evaluate, weight/normalization);
        }
      }
    }
  }
  _postProcessed = true;
}
