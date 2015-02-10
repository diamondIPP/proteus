#ifndef SINGLEANALYZER_H
#define SINGLEANALYZER_H

#include <vector>

#include <TDirectory.h>

namespace Storage { class Event; }
namespace Mechanics { class Device; }

namespace Analyzers {
  
  class EventCut;
  class TrackCut;
  class ClusterCut;
  class HitCut;
  
  class SingleAnalyzer {

    /**  Accessible to the derived class, but not externally */
  protected:
    SingleAnalyzer(const Mechanics::Device* device,
		   TDirectory* dir = 0,
		   const char* nameSuffix = "");    

    void validSensor(unsigned int nsensor);
    void eventDeviceAgree(const Storage::Event* event);
    TDirectory* makeGetDirectory(const char* dirName);

  public:
    virtual ~SingleAnalyzer();
    
    // This behaviour NEEDS to be implemented in the derived classes
    virtual void processEvent(const Storage::Event* event) = 0;
    virtual void postProcessing() = 0;
    
    void addCut(const EventCut* cut);
    void addCut(const TrackCut* cut);
    void addCut(const ClusterCut* cut);
    void addCut(const HitCut* cut);
    
  protected:
    const Mechanics::Device* _device;
    TDirectory* _dir;
    std::string _nameSuffix;
    bool _postProcessed;
    
    unsigned int _numEventCuts;    
    std::vector<const EventCut*> _eventCuts;
    
    unsigned int _numTrackCuts;    
    std::vector<const TrackCut*> _trackCuts;
    
    unsigned int _numClusterCuts;	
    std::vector<const ClusterCut*> _clusterCuts;
    
    unsigned int _numHitCuts;
    std::vector<const HitCut*> _hitCuts;
    
  }; // end of class
  
} // end of namespace

#endif // SINGLEANALYZER_H
