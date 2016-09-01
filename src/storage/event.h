#ifndef EVENT_H
#define EVENT_H

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "cluster.h"
#include "hit.h"
#include "plane.h"
#include "track.h"

namespace Processors {
class TrackMaker;
}

namespace Storage {
class Event {
public:
  Event(unsigned int numPlanes);

  Hit* newHit(unsigned int nplane);
  Cluster* newCluster(unsigned int nplane);
  Track* newTrack();

  Plane* getPlane(unsigned int n) { return &_planes.at(n); }
  Hit* getHit(unsigned int n) { return &_hits.at(n); }
  Cluster* getCluster(unsigned int n) { return &_clusters.at(n); }
  Track* getTrack(unsigned int n) { return &_tracks.at(n); }
  const Plane* getPlane(unsigned int n) const { return &_planes.at(n); }
  const Cluster* getCluster(unsigned int n) const { return &_clusters.at(n); }
  const Hit* getHit(unsigned int n) const { return &_hits.at(n); }
  const Track* getTrack(unsigned int n) const { return &_tracks.at(n); }

  void setInvalid(bool value) { _invalid = value; }
  void setTimeStamp(uint64_t timeStamp) { _timeStamp = timeStamp; }
  void setFrameNumber(uint64_t frameNumber) { _frameNumber = frameNumber; }
  void setTriggerOffset(unsigned int triggerOffset)
  {
    _triggerOffset = triggerOffset;
  }
  void setTriggerInfo(unsigned int triggerInfo) { _triggerInfo = triggerInfo; }
  void setTriggerPhase(unsigned int triggerPhase)
  {
    _triggerPhase = triggerPhase;
  }

  unsigned int getNumHits() const { return _hits.size(); }
  unsigned int getNumClusters() const { return _clusters.size(); }
  unsigned int getNumPlanes() const { return _planes.size(); }
  unsigned int getNumTracks() const { return _tracks.size(); }
  bool getInvalid() const { return _invalid; }
  uint64_t getTimeStamp() const { return _timeStamp; }
  uint64_t getFrameNumber() const { return _frameNumber; }
  unsigned int getTriggerOffset() const { return _triggerOffset; }
  unsigned int getTriggerInfo() const { return _triggerInfo; }
  unsigned int getTriggerPhase() const { return _triggerPhase; }

  friend class Processors::TrackMaker;

  void print(std::ostream& os, const std::string& prefix = std::string()) const;
  void print() const;

protected:
  void addTrack(Track* track);

private:
  uint64_t _timeStamp;
  uint64_t _frameNumber;
  unsigned int _triggerOffset;
  unsigned int _triggerInfo; // Dammit Andrej!
  unsigned int _triggerPhase;
  bool _invalid;

  std::vector<Hit> _hits;
  std::vector<Cluster> _clusters;
  std::vector<Plane> _planes;
  std::vector<Track> _tracks;
}; // end of class

} // end of namespace

#endif // EVENT_H
