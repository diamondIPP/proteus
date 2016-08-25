#include "dutresiduals.h"

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
// This header defines all the cuts
#include "cuts.h"

namespace Analyzers {

void DUTResiduals::processEvent(const Storage::Event *refEvent,
                                const Storage::Event *dutEvent)
{
  assert(refEvent && dutEvent && "Analyzer: can't process null events");

  // Throw an error for sensor / plane mismatch
  eventDeviceAgree(refEvent, dutEvent);

  //Max Cluster Size
  _maxclustersize = 5;
  // Check if the event passes the cuts
  for (unsigned int ncut = 0; ncut < _numEventCuts; ncut++)
    if (!_eventCuts.at(ncut)->check(refEvent)) return;

  //std::cout <<"Number of tracks = "<< refEvent->getNumTracks() << std::endl;

  for (unsigned int ntrack = 0; ntrack < refEvent->getNumTracks(); ntrack++)
  {
    const Storage::Track *track = refEvent->getTrack(ntrack);

    // Check if the track passes the cuts
    bool pass = true;
    for (unsigned int ncut = 0; ncut < _numTrackCuts; ncut++)
      if (!_trackCuts.at(ncut)->check(track)) {
        pass = false;
        break;
      }
    if (!pass) continue;

    for (unsigned int nplane = 0; nplane < dutEvent->getNumPlanes(); nplane++)
    {
      const Storage::Plane *plane = dutEvent->getPlane(nplane);
      const Mechanics::Sensor *sensor = _dutDevice->getSensor(nplane);
      double tx = 0, ty = 0, tz = 0;
      Processors::trackSensorIntercept(track, sensor, tx, ty, tz);

      //std::cout <<"Number of clusters = "<< plane->getNumClusters() << std::endl;

      for (unsigned int ncluster = 0; ncluster < plane->getNumClusters(); ncluster++)
      {
        const Storage::Cluster *cluster = plane->getCluster(ncluster);

        // Check if the cluster passes the cuts
        bool pass = true;
        for (unsigned int ncut = 0; ncut < _numClusterCuts; ncut++)
          if (!_clusterCuts.at(ncut)->check(cluster)) {
            pass = false;
            break;
          }
        //Bilbao@cern.ch:Cut for cluster size (also in efficiency to affect all eff.root results)
        //if(cluster->getNumHits()==1){ pass = false; break;}
        if (!pass) continue;

        const double rx = tx - cluster->getPosX();
        const double ry = ty - cluster->getPosY();
        //if(ry>100){ std::cout<<"Cluster rotto "<<cluster->getPixX()<<" "<<cluster->getPixY()<<std::endl;}
        //std::cout<<" for track "<< ntrack << " and cluster "<< ncluster << ", ry = "<< ry <<" and rx = "<< rx <<std::endl;

        //marco.rimoldi@cern.ch
        int clustersize = cluster->getNumHits();
        if (clustersize >= _maxclustersize) {
          clustersize = _maxclustersize;
        }



      double px = 0, py = 0;
      sensor->spaceToPixel(tx, ty, tz, px, py);




      const Storage::Hit* hit = cluster->getHit(0);
       // if (hit->getPixX()!=8 )continue;
        //std::cout<<clustersize<<std::endl
       // if(clustersize==1 && (ry>-200 && ry<-150)) std::cout<<tx<<" "<<cluster->getPosX()<<std::endl;
        (_residualsX_cluster.at(nplane))[clustersize-1]->Fill(rx);
        (_residualsY_cluster.at(nplane))[clustersize-1]->Fill(ry);
        //

        _residualsX.at(nplane)->Fill(rx);
        _residualsY.at(nplane)->Fill(ry);
        _distance.at(nplane)->Fill(sqrt(pow(rx, 2) + pow(ry, 2)));
        _residualsXX.at(nplane)->Fill(rx, tx);
        _residualsYY.at(nplane)->Fill(ry, ty);
        _residualsXY.at(nplane)->Fill(rx, ty);
        _residualsYX.at(nplane)->Fill(ry, tx);
      }
    }
  }
}

void DUTResiduals::postProcessing() { } // Needs to be declared even if not used

TH1D *DUTResiduals::getResidualX(unsigned int nsensor)
{
  validDutSensor(nsensor);
  return _residualsX.at(nsensor);
}

TH1D *DUTResiduals::getResidualY(unsigned int nsensor)
{
  validDutSensor(nsensor);
  return _residualsY.at(nsensor);
}

TH2D *DUTResiduals::getResidualXX(unsigned int nsensor)
{
  validDutSensor(nsensor);
  return _residualsXX.at(nsensor);
}

TH2D *DUTResiduals::getResidualXY(unsigned int nsensor)
{
  validDutSensor(nsensor);
  return _residualsXY.at(nsensor);
}

TH2D *DUTResiduals::getResidualYY(unsigned int nsensor)
{
  validDutSensor(nsensor);
  return _residualsYY.at(nsensor);
}

TH2D *DUTResiduals::getResidualYX(unsigned int nsensor)
{
  validDutSensor(nsensor);
  return _residualsYX.at(nsensor);
}

DUTResiduals::DUTResiduals(const Mechanics::Device *refDevice,
                           const Mechanics::Device *dutDevice,
                           TDirectory *dir,
                           const char *suffix,
                           unsigned int nPixX,
                           double binsPerPix,
                           unsigned int binsY) :
  // Base class is initialized here and manages directory / device
  DualAnalyzer(refDevice, dutDevice, dir, suffix)
{
  assert(refDevice && dutDevice && "Analyzer: can't initialize with null device");
  _maxclustersize = 5;
  // Makes or gets a directory called from inside _dir with this name
  TDirectory *dir1d = makeGetDirectory("DUTResiduals1D");
  TDirectory *dir2d = makeGetDirectory("DUTResiduals2D");

  std::stringstream name; // Build name strings for each histo
  std::stringstream title; // Build title strings for each histo

  std::stringstream xAxisTitle;
  std::stringstream yAxisTitle;

  // Generate a histogram for each sensor in the device
  for (unsigned int nsens = 0; nsens < _dutDevice->getNumSensors(); nsens++)
  {
    const Mechanics::Sensor *sensor = _dutDevice->getSensor(nsens);
    for (unsigned int axis = 0; axis < 2; axis++)
    {
      const double width = nPixX * (axis ? sensor->getPosPitchX() : sensor->getPosPitchY());
      unsigned int nbins = binsPerPix * nPixX;
      if (!(nbins % 2)) nbins += 1;
      double height = 0;

      xAxisTitle.str("");
      xAxisTitle << "Track cluster difference " << (axis ? "X" : "Y")
                 << " [" << _refDevice->getSpaceUnit() << "]";

      // Generate the 1D residual distribution for the given axis
      name.str(""); title.str("");
      name << sensor->getName()
           << ((axis) ? "X" : "Y") << _nameSuffix;
      title << sensor->getName()
            << ((axis) ? " X" : " Y");
      TH1D *res1d = new TH1D(name.str().c_str(), title.str().c_str(), 2 * nbins, -width, width);
      //bilbao@cern.ch
      //nbins, -width / 2.0, width / 2.0);
      res1d->SetDirectory(dir1d);
      res1d->GetXaxis()->SetTitle(xAxisTitle.str().c_str());
      if (axis) _residualsX.push_back(res1d);
      else      _residualsY.push_back(res1d);

      //marco.rimoldi@cern.ch
      std::vector<TH1D *> tmp_x;
      std::vector<TH1D *> tmp_y;
      for (int cl_size = 1; cl_size <= _maxclustersize; cl_size++) {
        name.str(""); title.str("");
        name << sensor->getName()
             << ((axis) ? "X" : "Y") << _nameSuffix << Form("_%d", cl_size);
        title << sensor->getName()
              << ((axis) ? " X" : " Y") << Form("_Cluster_Size_%d", cl_size);
        //std::cout<<name.str()<<std::endl;
       TH1D *res1d_cl = new TH1D(name.str().c_str(), title.str().c_str(), 2 * nbins, -width, width);
       //TRY
       // TH1D *res1d_cl = new TH1D(name.str().c_str(), title.str().c_str(), 20, -5, 5);
        if (!cl_size==0) res1d_cl->SetDirectory(dir1d);
        res1d_cl->GetXaxis()->SetTitle(xAxisTitle.str().c_str());
        if (axis) tmp_x.push_back(res1d_cl);
        else      tmp_y.push_back(res1d_cl);
      }

      if (axis) {
        _residualsX_cluster.push_back(tmp_x);
        tmp_x.clear();
      }
      else      {
        _residualsY_cluster.push_back(tmp_y);
        tmp_y.clear();
      }



      //bilbao@cern.ch
      //if condition to avoid filling it twice and declaring the variables again in another loop.
      if (axis) {
        xAxisTitle.str("");
        xAxisTitle << "Track to cluster distance " << " [" << _refDevice->getSpaceUnit() << "]";
        // Generate the 1D track to cluster distance distribution for the given axis
        name.str(""); title.str("");
        name << sensor->getName() << _nameSuffix << "Dist";
        title << sensor->getName() << "Track to cluster distance";
        TH1D *dist1d = new TH1D(name.str().c_str(), title.str().c_str(),
                                2 * nbins, 0, 2 * width);
        dist1d->SetDirectory(dir1d);
        dist1d->GetXaxis()->SetTitle(xAxisTitle.str().c_str());
        _distance.push_back(dist1d);
      }



      // Generate the XX or YY residual distribution

      // The height of this plot depends on the sensor and X or Y axis
      height = axis ? sensor->getSensitiveX() : sensor->getSensitiveY();
      yAxisTitle.str("");
      yAxisTitle << "Track position " << (axis ? "X" : "Y")
                 << " [" << _refDevice->getSpaceUnit() << "]";

      name.str(""); title.str("");
      name << sensor->getName()
           << ((axis) ? "XvsX" : "YvsY") << _nameSuffix;
      title << sensor->getName()
            << ((axis) ? " X vs. X" : " Y vs. Y");
      TH2D *resAA = new TH2D(name.str().c_str(), title.str().c_str(),
                             nbins, -width / 2.0, width / 2.0,
                             binsY, -height / 2.0, height / 2.0);
      resAA->SetDirectory(dir2d);
      resAA->GetXaxis()->SetTitle(xAxisTitle.str().c_str());
      resAA->GetYaxis()->SetTitle(yAxisTitle.str().c_str());
      if (axis) _residualsXX.push_back(resAA);
      else      _residualsYY.push_back(resAA);

      // Generate the XY or YX distribution

      height = axis ? sensor->getSensitiveY() : sensor->getSensitiveX();
      yAxisTitle.str("");
      yAxisTitle << "Track position " << (axis ? "Y" : "X")
                 << " [" << _refDevice->getSpaceUnit() << "]";

      name.str(""); title.str("");
      name << sensor->getName()
           << ((axis) ? "XvsY" : "YvsX") << _nameSuffix;
      title << sensor->getName()
            << ((axis) ? " X vs. Y" : " Y vs. X");

      TH2D *resAB = new TH2D(name.str().c_str(), title.str().c_str(),
                             nbins, -width / 2.0, width / 2.0,
                             binsY, -height / 2.0, height / 2.0);

      resAB->SetDirectory(dir2d);
      resAB->GetXaxis()->SetTitle(xAxisTitle.str().c_str());
      resAB->GetYaxis()->SetTitle(yAxisTitle.str().c_str());
      if (axis) _residualsXY.push_back(resAB);
      else      _residualsYX.push_back(resAB);
    }
  }
}

}
