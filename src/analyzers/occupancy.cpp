#include "occupancy.h"

#include <cassert>
#include <sstream>
#include <math.h>
#include <iostream>

#include <TDirectory.h>
#include <TH2D.h>
#include <TH1D.h>

#include "../mechanics/device.h"
#include "../mechanics/sensor.h"
#include "../storage/hit.h"
#include "../storage/cluster.h"
#include "../storage/plane.h"
#include "../storage/track.h"
#include "../storage/event.h"
#include "../processors/processors.h"

using namespace std;

//=========================================================
Analyzers::Occupancy::Occupancy(const Mechanics::Device* device,
				TDirectory* dir,
				const char* suffix) :
  SingleAnalyzer(device,dir,suffix, "Occupancy"), m_numEvents(0)
{
  assert(device && "Analyzer: can't initialize with null device");
  
  for(unsigned int nsens=0; nsens<device->getNumSensors(); nsens++)
    _totalHitOccupancy.push_back(0);
  
  bookHistos(makeGetDirectory("Occupancy"));
}

//=========================================================
void Analyzers::Occupancy::bookHistos(TDirectory* plotDir){
	std::string name, title;
  
  for (unsigned int nsens=0; nsens<_device->getNumSensors(); nsens++){
    const Mechanics::Sensor* sensor = _device->getSensor(nsens);

		auto areaPix = sensor->sensitiveAreaPixel(); // col_min,col_max,row_min,row_max
		auto areaXY = sensor->sensitiveEnvelopeGlobal(); // x_min, x_max, y_min, y_max

		// raw hits in pixel coordinates
		name = sensor->name() + "-HitMap";
		title = sensor->name() + " Hit Map";
    TH2D* hist = new TH2D(name.c_str(), title.c_str(),
                          sensor->numCols(), areaPix[0], areaPix[1],
                          sensor->numRows(), areaPix[2], areaPix[3]);
    hist->GetXaxis()->SetTitle("pixel column");
    hist->GetYaxis()->SetTitle("pixel row");
    hist->SetDirectory(plotDir);
    _hitOcc.push_back(hist);

		// clustered hits in pixel coordinates
		name = sensor->name() + "-ClusteredHitMap";
		title = sensor->name() + " Clustered Hit Map";
    TH2D* clusterHit = new TH2D(name.c_str(), title.c_str(),
                          sensor->numCols(), areaPix[0], areaPix[1],
                          sensor->numRows(), areaPix[2], areaPix[3]);
    clusterHit->GetXaxis()->SetTitle("pixel column");
    clusterHit->GetYaxis()->SetTitle("pixel row");
    clusterHit->SetDirectory(plotDir);
    _clusterHitOcc.push_back(clusterHit);

		// cluster positions in global xy coordinates
		name = sensor->name() + "-ClusterMap";
		title = sensor->name() + " Cluster Map";
    TH2D* histClust = new TH2D(name.c_str(), title.c_str(),
			       3 * sensor->numCols(), areaXY[0], areaXY[1],
			       3 * sensor->numRows(), areaXY[2], areaXY[3]);
    // sprintf(name,"x-position [%s]", _device->getSpaceUnit());
    histClust->GetXaxis()->SetTitle("global x position");
    // sprintf(name,"y-position [%s]", _device->getSpaceUnit());
    histClust->GetYaxis()->SetTitle("global y position");
    histClust->SetDirectory(plotDir);
    _clusterOcc.push_back(histClust);

		// occupancy distribution
		name = sensor->name() + "-HitOccupancy";
		title = sensor->name() + " Hit Occupancy distribution";
		TH1D *h = new TH1D(name.c_str(), title.c_str(), 100, 0, 1);
    //TH1D *h = new TH1D(hname, htitle, vMaxHits[nsens]+1, 0, vMaxHits[nsens]+1); // rebin offline if needed
    h->GetXaxis()->SetTitle("hits / event");
    h->SetDirectory(plotDir);
		_occDist.push_back(h);

  }
}

//=========================================================
TH2D* Analyzers::Occupancy::getHitOcc(unsigned int nsensor) {
  validSensor(nsensor);
  return _hitOcc.at(nsensor);
}

//=========================================================
TH1D* Analyzers::Occupancy::getHitOccDist(unsigned int nsensor){
  if(!_postProcessed)
    throw "Occupancy: requested plot needs to be generated by post-processing";
  validSensor(nsensor);
  return _occDist.at(nsensor);
}

//=========================================================
ULong64_t Analyzers::Occupancy::getTotalHitOccupancy(unsigned int nsensor){
  validSensor(nsensor);
  return _totalHitOccupancy.at(nsensor);
}

//=========================================================
void Analyzers::Occupancy::processEvent(const Storage::Event* event) {
  assert(event && "Analyzer: can't process null events");
  
  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(event);
  
  // Check if the event passes the cuts
  if (!checkCuts(event))
    return;
  
	m_numEvents += 1;

  // 0.- Loop in planes
  for (unsigned int nplane=0; nplane<event->numPlanes(); nplane++){
    const Storage::Plane* plane=event->getPlane(nplane);
    TH2D* hits = _hitOcc.at(nplane);
		TH2D* clusteredHits = _clusterHitOcc.at(nplane);
		TH2D* clusters = _clusterOcc.at(nplane);
		
    // 1.- Loop in hits within plane and fill histos
    for (unsigned int nhit=0; nhit<plane->numHits(); nhit++){
      const Storage::Hit* hit = plane->getHit(nhit);
      
      // Check if the hit passes the cuts
      if (!checkCuts(hit))
        continue;
      
      hits->Fill(hit->col(), hit->row());
      _totalHitOccupancy.at(nplane)++;
    }
    
    // 2.- Loop in clusters within plane and fill histos
    for (unsigned int ncluster=0; ncluster<plane->numClusters(); ncluster++){
      const Storage::Cluster* cluster = plane->getCluster(ncluster);
      
      // Check if the cluster passes the cuts
      if (!checkCuts(cluster))
        continue;

      clusters->Fill(cluster->posGlobal().x(), cluster->posGlobal().y());
			
			for (Index ihit = 0; ihit < cluster->numHits(); ++ihit) {
				const Storage::Hit* hit = cluster->getHit(ihit);
				clusteredHits->Fill(hit->col(), hit->row());
			}
    }
  }
}

void Analyzers::Occupancy::postProcessing()
{
  for (unsigned int nsens = 0; nsens < _device->getNumSensors(); nsens++) {
    const Mechanics::Sensor* sensor = _device->getSensor(nsens);

    TH2D* hits = _hitOcc.at(nsens);
    TH1D* pixels = _occDist.at(nsens);

    // rebin histogram to maximum occupancy range
    pixels->SetBins(100, 0, hits->GetMaximum() / m_numEvents);
    pixels->Reset();
    for (int bx = 1; bx <= hits->GetNbinsX(); ++bx) {
      for (int by = 1; by <= hits->GetNbinsY(); ++by) {
        double numHits = hits->GetBinContent(bx, by);
        if (numHits != 0)
          pixels->Fill(numHits / m_numEvents);
      }
    }
  }
