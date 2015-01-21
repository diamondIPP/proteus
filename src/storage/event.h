#ifndef EVENT_H
#define EVENT_H

#include <vector>

#include <Rtypes.h>

namespace Processors { class TrackMaker; }

namespace Storage
{

class Hit;
class Cluster;
class Track;
class Plane;
 
 class Event {
 public:
   Event(unsigned int numPlanes);
   ~Event();
   
   void print();
   
   Hit* newHit(unsigned int nplane);
   Cluster* newCluster(unsigned int nplane);
   Track* newTrack();
   
   Hit* getHit(unsigned int n) const;
   Cluster* getCluster(unsigned int n) const;
   Plane* getPlane(unsigned int n) const;
   Track* getTrack(unsigned int n) const;
   
   void setInvalid(bool value) { _invalid = value; }
   void setTimeStamp(ULong64_t timeStamp) { _timeStamp = timeStamp; }
   void setFrameNumber(ULong64_t frameNumber) { _frameNumber = frameNumber; }
   void setTriggerOffset(unsigned int triggerOffset) { _triggerOffset = triggerOffset; }
   void setTriggerInfo(unsigned int triggerInfo) { _triggerInfo = triggerInfo; }
   
   unsigned int getNumHits() const { return _numHits; }
   unsigned int getNumClusters() const { return _numClusters; }
   unsigned int getNumPlanes() const { return _numPlanes; }
   unsigned int getNumTracks() const { return _numTracks; }
   bool getInvalid() const { return _invalid; }
   ULong64_t getTimeStamp() const { return _timeStamp; }
   ULong64_t getFrameNumber() const { return _frameNumber; }
   unsigned int getTriggerOffset() const { return _triggerOffset; }
   unsigned int getTriggerInfo() const { return _triggerInfo; }
   
   friend class Processors::TrackMaker;
   
 protected:
   void addTrack(Track* track);
   
 private:
   ULong64_t _timeStamp;
   ULong64_t _frameNumber;
   unsigned int _triggerOffset;
   unsigned int _triggerInfo; // Dammit Andrej!
   bool _invalid;
   
   unsigned int _numHits;
   std::vector<Hit*> _hits;
   
   unsigned int _numClusters;
   std::vector<Cluster*> _clusters;
   
   unsigned int _numPlanes;
   std::vector<Plane*> _planes;
   
   unsigned int _numTracks;
   std::vector<Track*> _tracks;

 }; // end of class
 
} // end of namespace

#endif // EVENT_H
