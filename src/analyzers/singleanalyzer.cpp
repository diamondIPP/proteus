#include "singleanalyzer.h"

#include <vector>

#include <TDirectory.h>

#include "../storage/event.h"
#include "../mechanics/device.h"
#include "cuts.h"

Analyzers::SingleAnalyzer::SingleAnalyzer(const Mechanics::Device* device,
					  TDirectory* dir,
					  const char* nameSuffix) :
  _device(device),
  _dir(dir),
  _nameSuffix(nameSuffix),
  _postProcessed(false),
  _numEventCuts(0),
  _numTrackCuts(0),
  _numClusterCuts(0),
  _numHitCuts(0)
{ }


Analyzers::SingleAnalyzer::~SingleAnalyzer() {
  for(unsigned int i=0; i<_eventCuts.size(); i++)
    delete _eventCuts.at(i);
  for (unsigned int i=0; i<_trackCuts.size(); i++)
    delete _trackCuts.at(i);
  for (unsigned int i=0; i<_clusterCuts.size(); i++)
    delete _clusterCuts.at(i);
  for (unsigned int i=0; i<_hitCuts.size(); i++)
    delete _hitCuts.at(i);
}

void Analyzers::SingleAnalyzer::validSensor(unsigned int nsensor) {
  if(nsensor >= _device->getNumSensors())
    throw "Analyzer: requested sensor exceeds range";
}

void Analyzers::SingleAnalyzer::eventDeviceAgree(const Storage::Event* event) {
  if (event->getNumPlanes() != _device->getNumSensors())
    throw "Analyzer: event / device plane mis-match";
}

TDirectory* Analyzers::SingleAnalyzer::makeGetDirectory(const char* dirName){
  TDirectory* newDir = 0;
  if(_dir){
    newDir = _dir->GetDirectory(dirName);
    if(!newDir) newDir = _dir->mkdir(dirName);
  }
  return newDir;
}

void Analyzers::SingleAnalyzer::addCut(const EventCut* cut) {
  _eventCuts.push_back(cut);
  _numEventCuts++;
}

void Analyzers::SingleAnalyzer::addCut(const TrackCut* cut) {
  _trackCuts.push_back(cut);
  _numTrackCuts++;
}

void Analyzers::SingleAnalyzer::addCut(const ClusterCut* cut) {
  _clusterCuts.push_back(cut);
  _numClusterCuts++;
}

void Analyzers::SingleAnalyzer::addCut(const HitCut* cut) {
  _hitCuts.push_back(cut);
  _numHitCuts++;
}
