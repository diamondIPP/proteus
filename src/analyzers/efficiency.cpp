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

namespace Analyzers {

void Efficiency::processEvent(const Storage::Event* refEvent,
                              const Storage::Event* dutEvent)
{
  assert(refEvent && dutEvent && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeivceAgree(refEvent, dutEvent);

  // Check if the event passes the cuts
  for (unsigned int ncut = 0; ncut < _numEventCuts; ncut++)
    if (!_eventCuts.at(ncut)->check(refEvent)) return;

  for (unsigned int ntrack = 0; ntrack < refEvent->getNumTracks(); ntrack++)
  {
    Storage::Track* track = refEvent->getTrack(ntrack);

    // Check if the track passes the cuts
    bool pass = true;
    for (unsigned int ncut = 0; ncut < _numTrackCuts; ncut++)
      if (!_trackCuts.at(ncut)->check(track)) { pass = false; break; }
    if (!pass) continue;

    // Make a list of the planes with their matched cluster
    std::vector<Storage::Cluster*> matches;
    for (unsigned int nplane = 0; nplane < dutEvent->getNumPlanes(); nplane++)
      matches.push_back(0); // No matches

    // Get the matches from the track
    for (unsigned int nmatch = 0; nmatch < track->getNumMatchedClusters(); nmatch++)
    {
      Storage::Cluster* cluster = track->getMatchedCluster(nmatch);

      // Check if this cluster passes the cuts
      bool pass = true;
      for (unsigned int ncut = 0; ncut < _numClusterCuts; ncut++){
        if (!_clusterCuts.at(ncut)->check(cluster)) {
	  pass = false;
	  break; 
	}
      pass = true;
      matches.at(cluster->getPlane()->getPlaneNum()) = cluster;
      }
    }

    assert(matches.size() == _dutDevice->getNumSensors() &&
           _relativeToSensor < (int)_dutDevice->getNumSensors() &&
           "Efficiency: matches has the wrong size");

    // If asking for relative efficiency to a plane, look for a match in that one first     
    if (_relativeToSensor >= 0 && !matches.at(_relativeToSensor)) continue;

    for (unsigned int nsensor = 0; nsensor < _dutDevice->getNumSensors(); nsensor++)
    {
      Mechanics::Sensor* sensor = _dutDevice->getSensor(nsensor);

      double tx = 0, ty = 0, tz = 0;
      Processors::trackSensorIntercept(track, sensor, tx, ty, tz);

      double px = 0, py = 0;
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

      _inPixelEfficiency.at(nsensor)->Fill((bool)match,
                                           trackPosX * sensor->getPitchX(),
                                           trackPosY * sensor->getPitchY());

      if (match)
      {
        //_efficiencyMap.at(nsensor)->Fill(true, tx, ty); 
	
	//bilbao@cern.ch / reina.camacho@cern.ch: Filling the efficiency histogram with the cluster position instead of the track position
	_efficiencyMap.at(nsensor)->Fill(true, match->getPosX(), match->getPosY());
        _matchedTracks.at(nsensor)->Fill(1);
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

void Efficiency::postProcessing()
{
  if (_postProcessed) return;

  for (unsigned int nsensor = 0; nsensor < _dutDevice->getNumSensors(); nsensor++)
  {
    // Get efficiency per pixel
    TEfficiency* efficiency = _efficiencyMap.at(nsensor);
    const TH1* values = efficiency->GetTotalHistogram();
    TH1D* distribution = _efficiencyDistribution.at(nsensor);

    // Loop over all pixel groups
    for (Int_t binx = 1; binx <= values->GetNbinsX(); binx++)
    {
      for (Int_t biny = 1; biny <= values->GetNbinsY(); biny++)
      {
        const Int_t bin = values->GetBin(binx, biny);
        if (values->GetBinContent(binx, biny) < 1) continue;
        const double value = efficiency->GetEfficiency(bin);
        const double sigmaLow = efficiency->GetEfficiencyErrorLow(bin);
        const double sigmaHigh = efficiency->GetEfficiencyErrorUp(bin);

        // Find the probability of this pixel group being found in all bins of the distribution
        double normalization = 0;
        for (Int_t distBin = 1; distBin <= distribution->GetNbinsX(); distBin++)
        {
          const double evaluate = distribution->GetBinCenter(distBin);
          const double sigma = (evaluate < value) ? sigmaLow : sigmaHigh;
          const double weight = TMath::Gaus(evaluate, value, sigma);
          normalization += weight;
        }
        for (Int_t distBin = 1; distBin <= distribution->GetNbinsX(); distBin++)
        {
          const double evaluate = distribution->GetBinCenter(distBin);
          const double sigma = (evaluate < value) ? sigmaLow : sigmaHigh;
          const double weight = TMath::Gaus(evaluate, value, sigma);
          distribution->Fill(evaluate, weight / normalization);
        }
      }
    }
  }

  _postProcessed = true;
}

Efficiency::Efficiency(const Mechanics::Device* refDevice,
                       const Mechanics::Device* dutDevice,
                       TDirectory* dir,
                       const char* suffix,
                       int relativeToSensor,
                       unsigned int rebinX,
                       unsigned int rebinY,
                       unsigned int pixBinsX,
                       unsigned int pixBinsY) :
  // Base class is initialized here and manages directory / device
  DualAnalyzer(refDevice, dutDevice, dir, suffix),
  // Initialize processing parameters here
  _relativeToSensor(relativeToSensor)
{
  assert(refDevice && dutDevice && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* plotDir = makeGetDirectory("Efficiency");

  std::stringstream name; // Build name strings for each histo
  std::stringstream title; // Build title strings for each histo

  if (relativeToSensor >= (int)_dutDevice->getNumSensors())
    throw "Efficiency: relative sensor exceeds range";

  // Generate a histogram for each sensor in the device
  for (unsigned int nsens = 0; nsens < _dutDevice->getNumSensors(); nsens++)
  {
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
    name << sensor->getDevice()->getName() << sensor->getName()
         << "Map" << _nameSuffix;
    // Special titel includes axis
    title << sensor->getDevice()->getName() << " " << sensor->getName()
          << " Efficiency Map"
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
    title << sensor->getDevice()->getName() << " " << sensor->getName()
          << " Grouped Efficiency";
    TH1D* pixels = new TH1D(name.str().c_str(), title.str().c_str(),
                            20, 0, 1.001);
    pixels->GetXaxis()->SetTitle("Pixel group efficiency");
    pixels->GetYaxis()->SetTitle("Pixel groups / 0.05");
    pixels->SetDirectory(plotDir);
    _efficiencyDistribution.push_back(pixels);

    // Track matching initialization
    name.str(""); title.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
         << "MatchedTracks" << _nameSuffix;
    title << sensor->getDevice()->getName() << " " << sensor->getName()
          << " Matched Tracks";
    TH1D* matched = new TH1D(name.str().c_str(), title.str().c_str(),
                             2, 0 - 0.5, 2 - 0.5);
    matched->GetXaxis()->SetTitle("0 : unmatched tracks | 1 : matched tracks");
    matched->GetZaxis()->SetTitle("Tracks");
    matched->SetDirectory(plotDir);
    _matchedTracks.push_back(matched);

    name.str(""); title.str("");
    name << sensor->getDevice()->getName() << sensor->getName()
         <<  "InPixelEfficiency" << _nameSuffix;
    title << sensor->getDevice()->getName() << " " << sensor->getName()
          << " In Pixel Efficiency"
          << ";X position [" << _dutDevice->getSpaceUnit() << "]"
          << ";Y position [" << _dutDevice->getSpaceUnit() << "]"
          << ";Tracks";
    TEfficiency* inPixelEfficiency = new TEfficiency(name.str().c_str(), title.str().c_str(),
                                                     pixBinsX, 0, sensor->getPitchX(),
                                                     pixBinsY, 0, sensor->getPitchY());
    inPixelEfficiency->SetDirectory(plotDir);
    _inPixelEfficiency.push_back(inPixelEfficiency);

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

}

