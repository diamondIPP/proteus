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
  SingleAnalyzer(device,dir,suffix)
{
  assert(device && "Analyzer: can't initialize with null device");
  
  for(unsigned int nsens=0; nsens<device->getNumSensors(); nsens++)
    _totalHitOccupancy.push_back(0);
  
  bookHistos(makeGetDirectory("Occupancy"));
}

//=========================================================
void Analyzers::Occupancy::bookHistos(TDirectory* plotDir){
  char name[500], title[500];
  
  for (unsigned int nsens=0; nsens<_device->getNumSensors(); nsens++){
    const Mechanics::Sensor* sensor = _device->getSensor(nsens);
    
    // (1) pixel hitmap (pix-Y vs pix-X, PIXEL units)
    sprintf(name,"%s%s_%s%s", _device->getName(), sensor->getName(), "HitOccupancy", _nameSuffix.c_str());
    sprintf(title,"%s - %s [%s]", _device->getName(), sensor->getName(), "Hit Occupancy");
    TH2D* hist = new TH2D(name, title,
                          sensor->getNumX(), -0.5, sensor->getNumX()-0.5,
                          sensor->getNumY(), -0.5, sensor->getNumY()-0.5);
    hist->GetXaxis()->SetTitle("x-pixel");
    hist->GetYaxis()->SetTitle("y-pixel");
    hist->SetDirectory(plotDir);
    _hitOcc.push_back(hist);
    
    // (2) cluster positions (clus-Y vs clux-X, GLOBAL coords)
    const double lowX = sensor->getOffX() - sensor->getPosSensitiveX() / 2.0;
    const double uppX = sensor->getOffX() + sensor->getPosSensitiveX() / 2.0;
    const double lowY = sensor->getOffY() - sensor->getPosSensitiveY() / 2.0;
    const double uppY = sensor->getOffY() + sensor->getPosSensitiveY() / 2.0;

    sprintf(name,"%s_%s%s", sensor->getName(), "ClusterOccupancy", _nameSuffix.c_str());
    sprintf(title,"%s [%s]", sensor->getName(), "Cluster Occupancy");
    TH2D* histClust = new TH2D(name, title,
			       sensor->getPosNumX(), lowX, uppX,
			       sensor->getPosNumY(), lowY, uppY);
    sprintf(name,"x-position [%s]", _device->getSpaceUnit());
    histClust->GetXaxis()->SetTitle(name);
    sprintf(name,"y-position [%s]", _device->getSpaceUnit());
    histClust->GetYaxis()->SetTitle(name);
    histClust->SetDirectory(plotDir);
    _clusterOcc.push_back(histClust);

    /*    name.str(""); title.str("");
    name << sensor->getName() << "ClusterOccupancyXZ" << _nameSuffix;
    title << sensor->getName() << " Cluster OccupancyXZ"
          << ";X position [" << _device->getSpaceUnit() << "]"
          << ";Z position [" << _device->getSpaceUnit() << "]"
          << ";Clusters / pixel";
    TH2D* histClustXZ = new TH2D(name.str().c_str(), title.str().c_str(),
				 79*sensor->getPosNumX()/(uppX-lowX), 1, 80,
				 349*sensor->getPosNumY()/(uppY-lowY), 1, 350);
    cout << sensor->getPosNumX() << " " << uppX << " " << lowX << " " << 79*sensor->getPosNumX()/(uppX - lowX) << endl;
    cout << sensor->getPosNumY() << " " << uppY << " " << lowY << " " << sensor->getPosNumY()/(uppY - lowY) << endl;
    cout << "nbinsX = " << histClustXZ->GetXaxis()->GetNbins() << endl;
    cout << "nbinsY = " << histClustXZ->GetYaxis()->GetNbins() << endl;
    cout << "  - OK 3" << endl;
    histClustXZ->SetDirectory(plotDir);
    _clusterOccXZ.push_back(histClustXZ);
    */

    /*
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
    */

    sprintf(name,"%s%s_%s%s", _device->getName(), sensor->getName(), "ClusterOccupancyPix", _nameSuffix.c_str());
    sprintf(title,"%s - %s [%s[", _device->getName(), sensor->getName(), "Cluster Occupancy (pixels)");
    TH2D* histClustPix = new TH2D(name, title,
				  sensor->getNumX(), -0.5, sensor->getNumX()-0.5,
				  sensor->getNumY(), -0.5, sensor->getNumY()-0.5);
    histClustPix->GetXaxis()->SetTitle("x-pixel");
    histClustPix->GetYaxis()->SetTitle("y-pixel");
    histClustPix->SetDirectory(plotDir);
    _clusterOccPix.push_back(histClustPix);
    
  } // end loop in device sensors
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
  
  // 0.- Loop in planes
  for (unsigned int nplane=0; nplane<event->numPlanes(); nplane++){
    const Storage::Plane* plane=event->getPlane(nplane);
    
    // 1.- Loop in hits within plane and fill histos
    for (unsigned int nhit=0; nhit<plane->numHits(); nhit++){
      const Storage::Hit* hit = plane->getHit(nhit);
      
      // Check if the hit passes the cuts
      if (!checkCuts(hit))
        continue;
      
      _hitOcc.at(nplane)->Fill(hit->getPixX(), hit->getPixY());
      _totalHitOccupancy.at(nplane)++;
    }
    
    // 2.- Loop in clusters within plane and fill histos
    for (unsigned int ncluster=0; ncluster<plane->numClusters(); ncluster++){
      const Storage::Cluster* cluster = plane->getCluster(ncluster);
      
      // Check if the cluster passes the cuts
      if (!checkCuts(cluster))
        continue;
      
      _clusterOcc.at(nplane)->Fill(cluster->getPosX(), cluster->getPosY());
      //_clusterOccXZ.at(nplane)->Fill(cluster->getPosX(), cluster->getPosY());
      //_clusterOccYZ.at(nplane)->Fill(cluster->getPosY(), cluster->getPosZ());
      _clusterOccPix.at(nplane)->Fill(cluster->getPixX(), cluster->getPixY());
      
    } // end loop in clusters
   
  } // end loop in planes
}

//=========================================================
void Analyzers::Occupancy::postProcessing(){
  if (_postProcessed) return;

  // Generate the bounds of the 1D occupancy hist
  std::vector<unsigned int> vTotalHits;
  std::vector<unsigned int> vMaxHits;
  for(unsigned int nsens=0; nsens<_device->getNumSensors(); nsens++){
    const Mechanics::Sensor* sensor = _device->getSensor(nsens);
    TH2D* occ = _hitOcc.at(nsens);
    unsigned int cnt=0, max=0;
    for(unsigned int x=0; x<sensor->getNumX(); x++){
      for(unsigned int y=0; y<sensor->getNumY(); y++){
	const unsigned int numHits = occ->GetBinContent(x+1,y+1);
	cnt += numHits;
	if(numHits > max )
	  max = numHits;
      }
    }
    vTotalHits.push_back(cnt);
    vMaxHits.push_back(max);
  }

  //for(unsigned int nsens=0; nsens<_device->getNumSensors(); nsens++)
  //cout << "       - plane " << nsens << " totHits=" << vTotalHits[nsens]
  //<< " max=" << vMaxHits[nsens] << endl;
  
  char hname[100], htitle[300];
  TDirectory* plotDir = makeGetDirectory("Occupancy");
  for(unsigned int nsens=0; nsens<_device->getNumSensors(); nsens++){
    const Mechanics::Sensor* sensor = _device->getSensor(nsens);

    sprintf(hname,"%s%s_OccDist", _device->getName(), sensor->getName());
    sprintf(htitle, "%s - %s [Occupancy distribution]", _device->getName(), sensor->getName());
    float xmax = vTotalHits[nsens] != 0 ? (double)vMaxHits[nsens]/(double)vTotalHits[nsens] : vMaxHits[nsens];
    TH1D *h = new TH1D(hname, htitle, 100, 0, xmax);
    //TH1D *h = new TH1D(hname, htitle, vMaxHits[nsens]+1, 0, vMaxHits[nsens]+1); // rebin offline if needed
    h->GetXaxis()->SetTitle("Hits per trigger");
    h->SetDirectory(plotDir);
        
    TH2D* occ = _hitOcc.at(nsens);
    for(int bx=1; bx<=occ->GetNbinsX(); ++bx){
      for(int by=1; by<=occ->GetNbinsY(); ++by){
	const unsigned int numHits = occ->GetBinContent(bx,by);
	if( vTotalHits[nsens] != 0 ) h->Fill( (double)numHits / (double)vTotalHits[nsens] );
	//h->Fill( (double)numHits );
      }
    }
    _occDist.push_back(h);
    
  } // end loop in planes

  // clear vectors
  vTotalHits.clear();
  vMaxHits.clear();
  
  _postProcessed = true;
}
