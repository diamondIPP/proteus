#ifndef BASEANALYZER_H
#define BASEANALYZER_H

#include <vector>
#include <string>
#include <TDirectory.h>

namespace Storage { 
  class Event;
  class Track;
  class Cluster;
  class Hit; 
}

namespace Analyzers {
  
  class EventCut;
  class TrackCut;
  class ClusterCut;
  class HitCut;
  
  class BaseAnalyzer {

    /**  Accessible to the derived class, but not externally */
  protected:
    BaseAnalyzer(TDirectory* dir,
		 const char* nameSuffix,
		 const char* analyzerName);    
    
    TDirectory* makeGetDirectory(const char* dirName);
    
    virtual ~BaseAnalyzer();
    
  public:
    void setAnalyzerName(const std::string name);

    void addCut(const EventCut* cut);
    void addCut(const TrackCut* cut);
    void addCut(const ClusterCut* cut);
    void addCut(const HitCut* cut);

    virtual void print() const;
    virtual const std::string printStr() const;
    
  protected:
    TDirectory* _dir;
    std::string _nameSuffix;
    bool _postProcessed;
    std::string _analyzerName;
    
    bool checkCuts(const Storage::Event* event) const;
    bool checkCuts(const Storage::Track* track) const;
    bool checkCuts(const Storage::Cluster* cluster) const;
    bool checkCuts(const Storage::Hit* hit) const;

  private:
    std::vector<const EventCut*> _eventCuts;
    unsigned int _numEventCuts;    

    std::vector<const TrackCut*> _trackCuts;
    unsigned int _numTrackCuts;    
    
    std::vector<const ClusterCut*> _clusterCuts;
    unsigned int _numClusterCuts;	
    
    std::vector<const HitCut*> _hitCuts;
    unsigned int _numHitCuts;

  }; // end of class
  
} // end of namespace

#endif // BASEANALYZER_H
