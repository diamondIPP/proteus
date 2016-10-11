#include "singleanalyzer.h"

#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

#include <TDirectory.h>

#include "../storage/event.h"
#include "../mechanics/device.h"
#include "cuts.h"

Analyzers::BaseAnalyzer::BaseAnalyzer(TDirectory* dir,
				      const char* nameSuffix,
				      const char *analyzerName) :
  _dir(dir),
  _nameSuffix(nameSuffix),
  _postProcessed(false),
  _analyzerName(analyzerName),
  _numEventCuts(0),
  _numTrackCuts(0),
  _numClusterCuts(0),
  _numHitCuts(0)
{}


Analyzers::BaseAnalyzer::~BaseAnalyzer() {
  for(unsigned int i=0; i<_eventCuts.size(); i++)
    delete _eventCuts.at(i);
  for (unsigned int i=0; i<_trackCuts.size(); i++)
    delete _trackCuts.at(i);
  for (unsigned int i=0; i<_clusterCuts.size(); i++)
    delete _clusterCuts.at(i);
  for (unsigned int i=0; i<_hitCuts.size(); i++)
    delete _hitCuts.at(i);
}

TDirectory* Analyzers::BaseAnalyzer::makeGetDirectory(const char* dirName){
  TDirectory* newDir = 0;
  if(_dir){
    newDir = _dir->GetDirectory(dirName);
    if(!newDir) newDir = _dir->mkdir(dirName);
  }
  return newDir;
}

void Analyzers::BaseAnalyzer::setAnalyzerName(const std::string name) {
  _analyzerName = name;
}

void Analyzers::BaseAnalyzer::addCut(const EventCut* cut) {
  _eventCuts.push_back(cut);
  _numEventCuts++;
}

void Analyzers::BaseAnalyzer::addCut(const TrackCut* cut) {
  _trackCuts.push_back(cut);
  _numTrackCuts++;
}

void Analyzers::BaseAnalyzer::addCut(const ClusterCut* cut) {
  _clusterCuts.push_back(cut);
  _numClusterCuts++;
}

void Analyzers::BaseAnalyzer::addCut(const HitCut* cut) {
  _hitCuts.push_back(cut);
  _numHitCuts++;
}

void Analyzers::BaseAnalyzer::print() const {
  std::cout << printStr() << std::endl;
}

const std::string Analyzers::BaseAnalyzer::printStr() const {
  std::ostringstream out;
  out << "analyzer '" <<  _analyzerName  << "' ; ";
  out << "number of cuts (Evt,Trk,Clus,Hit) = ("
      <<  _numEventCuts << ","
      <<  _numTrackCuts << ","
      <<  _numClusterCuts << ","
      <<  _numHitCuts << ")";
 return out.str();
}

bool Analyzers::BaseAnalyzer::checkCuts(const Storage::Event* event) const {
  for (auto cut = _eventCuts.begin(); cut != _eventCuts.end(); ++cut) {
    if (!(*cut)->check(event)) {
      return false;
    }
  }
  return true;
}

bool Analyzers::BaseAnalyzer::checkCuts(const Storage::Track* track) const {
  for (auto cut = _trackCuts.begin(); cut != _trackCuts.end(); ++cut) {
    if (!(*cut)->check(track)) {
      return false;
    }
  }
  return true;
}

bool Analyzers::BaseAnalyzer::checkCuts(const Storage::Cluster* cluster) const {
  for (auto cut = _clusterCuts.begin(); cut != _clusterCuts.end(); ++cut) {
    if (!(*cut)->check(cluster)) {
      return false;
    }
  }
  return true;
}

bool Analyzers::BaseAnalyzer::checkCuts(const Storage::Hit* hit) const {
  for (auto cut = _hitCuts.begin(); cut != _hitCuts.end(); ++cut) {
    if (!(*cut)->check(hit)) {
      return false;
    }
  }
  return true;
}

