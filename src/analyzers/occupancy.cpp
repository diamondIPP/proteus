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
#include "cuts.h"

using namespace std;

//=========================================================
Analyzers::Occupancy::Occupancy(const Mechanics::Device* device,
				TDirectory* dir,
				const char* suffix) :
  SingleAnalyzer(device,dir,suffix),
  totalHitOccupancy(0)
{
  assert(device && "Analyzer: can't initialize with null device");
  
  TDirectory* plotDir = makeGetDirectory("Occupancy");
  
  std::stringstream name;  // Build name strings for each histo
  std::stringstream title; // Build title strings for each histo
  
  // Generate a histogram for each sensor in the device
  for (unsigned int nsens=0; nsens<_device->getNumSensors(); nsens++){
    Mechanics::Sensor* sensor = _device->getSensor(nsens);

    const double lowX = sensor->getOffX() - sensor->getPosSensitiveX() / 2.0;
    const double uppX = sensor->getOffX() + sensor->getPosSensitiveX() / 2.0;
    const double lowY = sensor->getOffY() - sensor->getPosSensitiveY() / 2.0;
    const double uppY = sensor->getOffY() + sensor->getPosSensitiveY() / 2.0;

    //
    // pixel hitmap (pix-Y vs pix-X, PIXEL units)
    //
    name.str(""); title.str("");
    name << sensor->getName() << "HitOccupancy" << _nameSuffix;
    title << sensor->getName() << " Pixel Occupancy"
          << ";X pixel"
          << ";Y pixel"
          << ";Hits / pixel";
    TH2D* hist = new TH2D(name.str().c_str(), title.str().c_str(),
                          sensor->getNumX(), -0.5, sensor->getNumX()-0.5,
                          sensor->getNumY(), -0.5, sensor->getNumY()-0.5);
    hist->SetDirectory(plotDir);
    _hitOcc.push_back(hist);
    
    //
    // (2) cluster positions (clus-Y vs clux-X, GLOBAL coords)
    //    
    name.str(""); title.str("");
    name << sensor->getName() << "ClusterOccupancy" << _nameSuffix;
    title << sensor->getName() << " Cluster Occupancy"
          << ";X position [" << _device->getSpaceUnit() << "]"
          << ";Y position [" << _device->getSpaceUnit() << "]"
          << ";Clusters / pixel";
    TH2D* histClust = new TH2D(name.str().c_str(), title.str().c_str(),
                               sensor->getPosNumX(), lowX, uppX,
                               sensor->getPosNumY(), lowY, uppY);
    //cout << "  - OK 2" << endl;
    histClust->SetDirectory(plotDir);
    _clusterOcc.push_back(histClust);

    //
    // do not understand this one...
    //
    name.str(""); title.str("");
    name << sensor->getName() << "ClusterOccupancyXZ" << _nameSuffix;
    title << sensor->getName() << " Cluster OccupancyXZ"
          << ";X position [" << _device->getSpaceUnit() << "]"
          << ";Z position [" << _device->getSpaceUnit() << "]"
          << ";Clusters / pixel";
    TH2D* histClustXZ = new TH2D(name.str().c_str(), title.str().c_str(),
				 79*sensor->getPosNumX()/(uppX-lowX), 1, 80,
				 349*sensor->getPosNumY()/(uppY-lowY), 1, 350);


    //cout << sensor->getPosNumX() << " " << uppX << " " << lowX << " " << 79*sensor->getPosNumX()/(uppX - lowX) << endl;
    //cout << sensor->getPosNumY() << " " << uppY << " " << lowY << " " << sensor->getPosNumY()/(uppY - lowY) << endl;

    //cout << "nbinsX = " << histClustXZ->GetXaxis()->GetNbins() << endl;
    //cout << "nbinsY = " << histClustXZ->GetYaxis()->GetNbins() << endl;
    
    //cout << "  - OK 3" << endl;
    histClustXZ->SetDirectory(plotDir);
    _clusterOccXZ.push_back(histClustXZ);
    
    name.str(""); title.str("");
    name << sensor->getName() << "ClusterOccupancyYZ" << _nameSuffix;
    title << sensor->getName() << " Cluster OccupancyYZ"
          << ";Y position [" << _device->getSpaceUnit() << "]"
          << ";Z position [" << _device->getSpaceUnit() << "]"
          << ";Clusters / pixel";
    TH2D* histClustYZ = new TH2D(name.str().c_str(), title.str().c_str(),
				 sensor->getPosNumY(), lowY, uppY,
				 sensor->getPosNumY()+500, sensor->getOffZ()+0.4, sensor->getOffZ()-0.4);
    histClustYZ->SetDirectory(plotDir);
    _clusterOccYZ.push_back(histClustYZ);


    //
    // do not understand this one...
    //
    name.str(""); title.str("");
    name << sensor->getName() << "ClusterOccupancyPix" << _nameSuffix;
    title << sensor->getName() << " Cluster Occupancy Pixel"
          << ";X pixel "
          << ";Y pixel"
          << ";Clusters / pixel";
    TH2D* histClustPix = new TH2D(name.str().c_str(), title.str().c_str(),
				  sensor->getNumX(), -0.5, sensor->getNumX()-0.5,
				  sensor->getNumY(), -0.5, sensor->getNumY()-0.5);
    histClustPix->SetDirectory(plotDir);
    _clusterOccPix.push_back(histClustPix);
    
  } // end loop in device sensors
}

//=========================================================
void Analyzers::Occupancy::processEvent(const Storage::Event* event) {
  assert(event && "Analyzer: can't process null events");
  
  // Throw an error for sensor / plane mismatch
  eventDeivceAgree(event);
  
  // Check if the event passes the cuts
  for (unsigned int ncut=0; ncut<_numEventCuts; ncut++)
    if (!_eventCuts.at(ncut)->check(event)) return;
  
  // (0) Loop in planes and fill histos  
  for (unsigned int nplane=0; nplane<event->getNumPlanes(); nplane++)
    {
      Storage::Plane* plane=event->getPlane(nplane);
      
      // (1) Loop in hits within plane and fill histos      
      for (unsigned int nhit=0; nhit<plane->getNumHits(); nhit++){
	Storage::Hit* hit = plane->getHit(nhit);
	
	// Check if the hit passes the cuts
	bool pass = true;
	for (unsigned int ncut=0; ncut<_numHitCuts; ncut++)
	  if (!_hitCuts.at(ncut)->check(hit)) { pass = false; break; }
	if (!pass) continue;
	
	totalHitOccupancy++;
	_hitOcc.at(nplane)->Fill(hit->getPixX(), hit->getPixY());
      }
      
      
      // (2) Loop in clusters within plane (global coordinates) and fill histos
      for (unsigned int ncluster=0; ncluster<plane->getNumClusters(); ncluster++){
	Storage::Cluster* cluster = plane->getCluster(ncluster);
	
	// Check if the cluster passes the cuts
	bool pass = true;
	for (unsigned int ncut=0; ncut<_numClusterCuts; ncut++)
	  if (!_clusterCuts.at(ncut)->check(cluster)) { pass = false; break; }
	if (!pass) continue;
	
	_clusterOcc.at(nplane)->Fill(cluster->getPosX(), cluster->getPosY());
	_clusterOccPix.at(nplane)->Fill(cluster->getPixX(), cluster->getPixY());
	_clusterOccXZ.at(nplane)->Fill(cluster->getPosX(), cluster->getPosY());  
	_clusterOccYZ.at(nplane)->Fill(cluster->getPosY(), cluster->getPosZ());
      }      
    } // end loop in planes
}

//=======================================
//
// Occupancy::postProcessing 
//
// (not used in the noise-scan to
// determine noisy pixels)
//
//=======================================
void Analyzers::Occupancy::postProcessing()
{
  if (_postProcessed) return;
  
  // Generate the bounds of the 1D occupancy hist
  unsigned int totalHits = 0;
  unsigned int maxHits = 0;

  for(unsigned int nsens=0; nsens<_device->getNumSensors(); nsens++){
    Mechanics::Sensor* sensor = _device->getSensor(nsens);
    TH2D* occ = _hitOcc.at(nsens);    
    for(unsigned int x=0; x<sensor->getNumX(); x++){
      for(unsigned int y=0; y<sensor->getNumY(); y++){
	const unsigned int numHits = occ->GetBinContent(x+1,y+1);
	totalHits += numHits;
	if(numHits > maxHits) maxHits = numHits;
      }
    }    
  }

  TDirectory* plotDir = makeGetDirectory("Occupancy");
  
  std::stringstream name;
  std::stringstream title;  
  name << "OccupancyDistribution";
  title << "Occupancy Distribution";  
  _occDistribution = new TH1D(name.str().c_str(), title.str().c_str(),
                              100, 0, (double)maxHits / (double)totalHits);  
  _occDistribution->SetDirectory(_dir);
  _occDistribution->GetXaxis()->SetTitle("Hits per trigger");
  _occDistribution->GetYaxis()->SetTitle("Pixels");
  _occDistribution->SetDirectory(plotDir);
  
  // Fill the occupancy distribution
  for (unsigned int nsens=0; nsens<_device->getNumSensors(); nsens++){
      Mechanics::Sensor* sensor = _device->getSensor(nsens);
      TH2D* occ = _hitOcc.at(nsens);      
      for(unsigned int x=0; x<sensor->getNumX(); x++){
	for(unsigned int y=0; y<sensor->getNumY(); y++){
	  const unsigned int numHits = occ->GetBinContent(x+1, y+1);
	  _occDistribution->Fill((double)numHits / (double)totalHits);
	}
      }
  }
  _postProcessed = true;
}

 //=========================================================
TH1D* Analyzers::Occupancy::getOccDistribution(){
  if (!_postProcessed)
    throw "Occupancy: requested plot needs to be generated by post-processing";
  return _occDistribution;
}

//=========================================================
TH2D* Analyzers::Occupancy::getHitOcc(unsigned int nsensor)
{
  validSensor(nsensor);
  return _hitOcc.at(nsensor);
}
