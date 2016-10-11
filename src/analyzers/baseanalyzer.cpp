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
  _analyzerName(analyzerName)
{}


Analyzers::BaseAnalyzer::~BaseAnalyzer() {
   for (auto ecut = _eventCuts.begin(); ecut != _eventCuts.end(); ++ecut)
      delete *ecut;
   for (auto tcut = _trackCuts.begin(); tcut != _trackCuts.end(); ++tcut)
      delete *tcut;
   for (auto ccut = _clusterCuts.begin(); ccut != _clusterCuts.end(); ++ccut)
      delete *ccut;
   for (auto hcut = _hitCuts.begin(); hcut != _hitCuts.end(); ++hcut)
      delete *hcut;
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
}

void Analyzers::BaseAnalyzer::addCut(const TrackCut* cut) {
  _trackCuts.push_back(cut);
}

void Analyzers::BaseAnalyzer::addCut(const ClusterCut* cut) {
  _clusterCuts.push_back(cut);
}

void Analyzers::BaseAnalyzer::addCut(const HitCut* cut) {
  _hitCuts.push_back(cut);
}

void Analyzers::BaseAnalyzer::print() const {
  std::cout << printStr() << std::endl;
}

const std::string Analyzers::BaseAnalyzer::printStr() const {
  std::ostringstream out;
  out << "analyzer '" <<  _analyzerName  << "' ; ";
  out << "number of cuts (Evt,Trk,Clus,Hit) = ("
      <<  _eventCuts.size() << ","
      <<  _trackCuts.size() << ","
      <<  _clusterCuts.size() << ","
      <<  _hitCuts.size() << ")";
 return out.str();
}

template<class TO, class TC>
bool checkCuts(const TO *obj, const TC& cuts) {
  for (auto cut = cuts.begin(); cut != cuts.end(); ++cut) {
    if (!(*cut)->check(obj)) {
      return false;
    }
  }
  return true;
}

bool Analyzers::BaseAnalyzer::checkCuts(const Storage::Event* event) const {
  return ::checkCuts(event, _eventCuts);
}

bool Analyzers::BaseAnalyzer::checkCuts(const Storage::Track* track) const {
  return ::checkCuts(track, _trackCuts);
}

bool Analyzers::BaseAnalyzer::checkCuts(const Storage::Cluster* cluster) const {
  return ::checkCuts(cluster, _clusterCuts);
}

bool Analyzers::BaseAnalyzer::checkCuts(const Storage::Hit* hit) const {
  return ::checkCuts(hit, _hitCuts);
}


