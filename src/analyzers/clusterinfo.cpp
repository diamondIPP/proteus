#include "clusterinfo.h"

#include <cassert>
#include <sstream>
#include <math.h>

#include <TDirectory.h>
#include <TH2D.h>
#include <TH1D.h>

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

//=========================================================
Analyzers::ClusterInfo::ClusterInfo(const Mechanics::Device* device,
                         TDirectory* dir,
                         const char* suffix,
                         unsigned int totBins,
                         unsigned int maxClusterSize) :
  // Base class is initialized here and manages directory / device
  SingleAnalyzer(device, dir, suffix),
  _totBins(totBins)
{
  assert(device && "Analyzer: can't initialize with null device");

  // Makes or gets a directory called from inside _dir with this name
  TDirectory* plotDir = makeGetDirectory("ClusterInfo");

  std::stringstream name; // Build name strings for each histo
  std::stringstream title; // Build title strings for each histo

  // Generate a histogram for each sensor in the device
  for (unsigned int nsens = 0; nsens < _device->getNumSensors(); nsens++)
  {
    const Mechanics::Sensor* sensor = _device->getSensor(nsens);

    name.str(""); title.str("");
    name << _device->getName() << sensor->getName()
         <<  "Size" << _nameSuffix;
    title << _device->getName() << " " << sensor->getName()
          << " Cluster Size"
          << ";Pixels in cluster"
          << ";Clusters";
    TH1D* size = new TH1D(name.str().c_str(), title.str().c_str(),
                          maxClusterSize - 1, 1 - 0.5, maxClusterSize - 0.5);
    size->SetDirectory(plotDir);
    _clusterSize.push_back(size);

    name.str(""); title.str("");
    name << _device->getName() << sensor->getName()
         <<  "ToT" << _nameSuffix;
    title << _device->getName() << " " << sensor->getName()
          << " Clustered ToT Distribution"
          << ";ToT bin number"
          << ";Clusters";
    TH1D* tot = new TH1D(name.str().c_str(), title.str().c_str(),
                         _totBins, 0 - 0.5, _totBins - 0.5);
    tot->SetDirectory(plotDir);
    _tot.push_back(tot);


    name.str(""); title.str("");
    name << _device->getName() << sensor->getName()
         << "TimingVsSize" << _nameSuffix;
    title << _device->getName() << " " << sensor->getName()
          << " Hit Timing in cluster Vs. Cluster Size"
          << ";Pixels in cluster"
          << ";Pixel timing [BC]"
          << ";Hits";
    TH2D* timing = new TH2D(name.str().c_str(), title.str().c_str(),
                             maxClusterSize - 1, 1 - 0.5, maxClusterSize - 0.5,
                             16, 0 - 0.5, 16 - 0.5);
    timing->SetDirectory(plotDir);
    _timingVsClusterSize.push_back(timing);

    //Bilbao@cern.ch: Not sure if this is the best place since it concerns timing for hits
    name.str(""); title.str("");
    name << _device->getName() << sensor->getName()
         << "TimingVsValue" << _nameSuffix;
    title << _device->getName() << " " << sensor->getName()
          << " Hit Timing in cluster Vs. Hit Value"
          << ";ToT"
          << ";Pixel timing [BC]"
          << ";Hits";
    TH2D* timingVsValue = new TH2D(name.str().c_str(), title.str().c_str(),
                             14, 1 - 0.5, 15 - 0.5,
                             16, 0 - 0.5, 16 - 0.5);
    timingVsValue->SetDirectory(plotDir);
    _timingVsHitValue.push_back(timingVsValue);

    name.str(""); title.str("");
    name << _device->getName() << sensor->getName()
         << "ToTVsSize" << _nameSuffix;
    title << _device->getName() << " " << sensor->getName()
          << " ToT Vs. Cluster Size"
          << ";Pixels in cluster"
          << ";ToT bin number"
          << ";Clusters";
    TH2D* totSize = new TH2D(name.str().c_str(), title.str().c_str(),
                             maxClusterSize - 1, 1 - 0.5, maxClusterSize - 0.5,
                             _totBins, 0 - 0.5, _totBins - 0.5);
    totSize->SetDirectory(plotDir);
    _totSize.push_back(totSize);


    if (_device->getTimeEnd() > _device->getTimeStart()) // If not used, they are both == 0
    {
      // Prevent aliasing
      const unsigned int nTimeBins = 100;
      const ULong64_t timeSpan = _device->getTimeEnd() - _device->getTimeStart() + 1;
      const ULong64_t startTime = _device->getTimeStart();
      const ULong64_t endTime = timeSpan - (timeSpan % nTimeBins) + startTime;

      name.str(""); title.str("");
      name << _device->getName() << sensor->getName()
           << "ClustersVsTime" << _nameSuffix;
      title << _device->getName() << " " << sensor->getName()
            << " Clustsers Vs. Time";
      TH1D* clusterTime = new TH1D(name.str().c_str(), title.str().c_str(),
                                   nTimeBins,
                                   _device->tsToTime(startTime),
                                   _device->tsToTime(endTime + 1));
      clusterTime->SetDirectory(0);
      _clustersVsTime.push_back(clusterTime);

      name.str(""); title.str("");
      name << _device->getName() << sensor->getName()
           << "TotVsTime" << _nameSuffix;
      title << _device->getName() << " " << sensor->getName()
            << " ToT Vs. Time"
            << ";Time [" << _device->getTimeUnit() << "]"
            << ";Average cluster ToT";
      TH1D* totTime = new TH1D(name.str().c_str(), title.str().c_str(),
                               nTimeBins,
                               _device->tsToTime(startTime),
                               _device->tsToTime(endTime + 1));
      totTime->SetDirectory(plotDir);
      _totVsTime.push_back(totTime);
    }
  }
}

//=========================================================
void Analyzers::ClusterInfo::processEvent(const Storage::Event* event) {
  assert(event && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);
  
  // Check if the event passes the cuts
  if (!checkCuts(event))
    return;

  for(unsigned int nplane=0; nplane<event->numPlanes(); nplane++) {
    const Storage::Plane* plane = event->getPlane(nplane);
    
    for(unsigned int ncluster=0; ncluster<plane->numClusters(); ncluster++){
      const Storage::Cluster* cluster = plane->getCluster(ncluster);
      
      // Check if the cluster passes the cuts
      if (!checkCuts(cluster))
        continue;
      
      _clusterSize.at(nplane)->Fill(cluster->getNumHits());
      _tot.at(nplane)->Fill(cluster->getValue());
      _totSize.at(nplane)->Fill(cluster->getNumHits(), cluster->getValue());
      
      for( unsigned int nhits=0; nhits<cluster->getNumHits(); nhits++){
	const Storage::Hit* hit= cluster->getHit(nhits);
	//std::cout << hit->getTiming() << std::endl;
	_timingVsClusterSize.at(nplane)->Fill(cluster->getNumHits(),hit->getTiming());
        _timingVsHitValue.at(nplane)->Fill(hit->getValue(),hit->getTiming());
      }

      if (_clustersVsTime.size())
        _clustersVsTime.at(nplane)->Fill(_device->tsToTime(event->getTimeStamp()));

      if (_totVsTime.size())
        _totVsTime.at(nplane)->Fill(_device->tsToTime(event->getTimeStamp()), cluster->getValue());
    }
  }
}

//=========================================================
void Analyzers::ClusterInfo::postProcessing(){
  if (_postProcessed) return;
  
  if (_clustersVsTime.size()) {
    for (unsigned int nsens=0; nsens<_device->getNumSensors(); nsens++){
      TH1D* hist = _totVsTime.at(nsens);
      TH1D* cnt  = _clustersVsTime.at(nsens);
      for(Int_t bin = 1; bin <= hist->GetNbinsX(); bin++) {
        if (cnt->GetBinContent(bin) < 1) continue;
        hist->SetBinContent(bin, hist->GetBinContent(bin) / (double)cnt->GetBinContent(bin));
      }
    }
  }
  _postProcessed = true;
}
