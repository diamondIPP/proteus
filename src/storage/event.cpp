#include "event.h"

#include <cassert>
#include <vector>
#include <iostream>

#include <Rtypes.h>

#include "hit.h"
#include "cluster.h"
#include "track.h"
#include "plane.h"

using std::cout;
using std::endl;

using Storage::Hit;
using Storage::Cluster;
using Storage::Track;
using Storage::Plane;

//=========================================================  
Storage::Event::Event(unsigned int numPlanes) :
  _timeStamp(0),
  _frameNumber(0),
  _triggerOffset(0),
  _invalid(false)
{
  for (unsigned int nplane=0; nplane< getNumPlanes(); nplane++){
    _planes.push_back(new Plane(nplane));
  }
}

//=========================================================
Storage::Event::~Event() {
  for(unsigned int nhit=0; nhit< getNumHits(); nhit++)
    delete _hits.at(nhit);
  
  for(unsigned int ncluster=0; ncluster< getNumClusters(); ncluster++)
    delete _clusters.at(ncluster);
  
  for(unsigned int nplane=0; nplane< getNumPlanes(); nplane++)
    delete _planes.at(nplane);
  
  for(unsigned int ntrack=0; ntrack< getNumTracks(); ntrack++)
    delete _tracks.at(ntrack);
}

//=========================================================
void Storage::Event::print(){
  cout << "\nEVENT:\n"
       << "  Time stamp: " << getTimeStamp() << "\n"
       << "  Frame number: " << getFrameNumber() << "\n"
       << "  Trigger offset: " << getTriggerOffset() << "\n"
       << "  Invalid: " << getInvalid() << "\n"
       << "  Num planes: " << getNumPlanes() << "\n"
       << "  Num hits: " << getNumHits() << "\n"
       << "  Num clusters: " << getNumClusters() << endl;
  
  for(unsigned int nplane=0; nplane < getNumPlanes(); nplane++){
    cout << "\n[" << nplane << "] "; getPlane(nplane)->print();
  }
  
}

//=========================================================
Storage::Hit* Storage::Event::newHit(unsigned int nplane) {
  Hit* hit = new Hit();
  _hits.push_back(hit);
  _planes.at(nplane)->addHit(hit);
  return hit;
}

//=========================================================
Cluster* Storage::Event::newCluster(unsigned int nplane) {
  Cluster* cluster = new Cluster();
  cluster->_index = _clusters.size();
  _clusters.push_back(cluster);
  _planes.at(nplane)->addCluster(cluster);
  return cluster;
}

//=========================================================
void Storage::Event::addTrack(Track* track){
  track->_index = _tracks.size();
  _tracks.push_back(track);
}

//=========================================================
Track* Storage::Event::newTrack(){
  Track* track = new Track();
  addTrack(track);
  return track;
}

//=========================================================
Hit* Storage::Event::getHit(unsigned int n) const {
  return _hits.at(n);
}

//=========================================================
Cluster* Storage::Event::getCluster(unsigned int n) const{
  return _clusters.at(n);
}

//=========================================================
Plane* Storage::Event::getPlane(unsigned int n) const {
  return _planes.at(n);
}

//=========================================================
Track* Storage::Event::getTrack(unsigned int n) const{
  return _tracks.at(n);
}

