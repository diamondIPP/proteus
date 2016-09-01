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

  Plane* getPlane(unsigned int n) { return &m_planes.at(n); }
  Hit* getHit(unsigned int n) { return &m_hits.at(n); }
  Cluster* getCluster(unsigned int n) { return &m_clusters.at(n); }
  Track* getTrack(unsigned int n) { return &m_tracks.at(n); }
  const Plane* getPlane(unsigned int n) const { return &m_planes.at(n); }
  const Cluster* getCluster(unsigned int n) const { return &m_clusters.at(n); }
  const Hit* getHit(unsigned int n) const { return &m_hits.at(n); }
  const Track* getTrack(unsigned int n) const { return &m_tracks.at(n); }

  void setInvalid(bool value) { m_invalid = value; }
  void setTimeStamp(uint64_t timeStamp) { m_timeStamp = timeStamp; }
  void setFrameNumber(uint64_t frameNumber) { m_frameNumber = frameNumber; }
  void setTriggerOffset(unsigned int triggerOffset)
  {
    m_triggerOffset = triggerOffset;
  }
  void setTriggerInfo(unsigned int triggerInfo) { m_triggerInfo = triggerInfo; }
  void setTriggerPhase(unsigned int triggerPhase)
  {
    m_triggerPhase = triggerPhase;
  }

  unsigned int getNumHits() const { return m_hits.size(); }
  unsigned int getNumClusters() const { return m_clusters.size(); }
  unsigned int getNumPlanes() const { return m_planes.size(); }
  unsigned int getNumTracks() const { return m_tracks.size(); }
  bool getInvalid() const { return m_invalid; }
  uint64_t getTimeStamp() const { return m_timeStamp; }
  uint64_t getFrameNumber() const { return m_frameNumber; }
  unsigned int getTriggerOffset() const { return m_triggerOffset; }
  unsigned int getTriggerInfo() const { return m_triggerInfo; }
  unsigned int getTriggerPhase() const { return m_triggerPhase; }

  void print(std::ostream& os, const std::string& prefix = std::string()) const;
  void print() const;

private:
  void addTrack(Track* track);

  uint64_t m_timeStamp;
  uint64_t m_frameNumber;
  unsigned int m_triggerOffset;
  unsigned int m_triggerInfo; // Dammit Andrej!
  unsigned int m_triggerPhase;
  bool m_invalid;

  std::vector<Hit> m_hits;
  std::vector<Cluster> m_clusters;
  std::vector<Plane> m_planes;
  std::vector<Track> m_tracks;

  friend class Processors::TrackMaker;
};

} // namespace Storage

#endif // EVENT_H
