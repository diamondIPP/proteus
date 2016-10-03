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
  Event(Index numPlanes);

  void setInvalid(bool value) { m_invalid = value; }
  void setTimeStamp(uint64_t timeStamp) { m_timeStamp = timeStamp; }
  void setFrameNumber(uint64_t frameNumber) { m_frameNumber = frameNumber; }
  void setTriggerOffset(unsigned int offset) { m_triggerOffset = offset; }
  void setTriggerInfo(unsigned int info) { m_triggerInfo = info; }
  void setTriggerPhase(unsigned int phase) { m_triggerPhase = phase; }

  bool invalid() const { return m_invalid; }
  uint64_t timeStamp() const { return m_timeStamp; }
  uint64_t frameNumber() const { return m_frameNumber; }
  unsigned int triggerOffset() const { return m_triggerOffset; }
  unsigned int triggerInfo() const { return m_triggerInfo; }
  unsigned int triggerPhase() const { return m_triggerPhase; }

  Index numPlanes() const { return static_cast<Index>(m_planes.size()); }
  Plane* getPlane(unsigned int n) { return &m_planes.at(n); }
  const Plane* getPlane(unsigned int n) const { return &m_planes.at(n); }

  Hit* newHit(Index iplane);
  Index numHits() const { return static_cast<Index>(m_hits.size()); }
  Hit* getHit(Index i) { return &m_hits.at(i); }
  const Hit* getHit(Index i) const { return &m_hits.at(i); }

  Cluster* newCluster(Index iplane);
  Index numClusters() const { return static_cast<Index>(m_clusters.size()); }
  Cluster* getCluster(Index i) { return &m_clusters.at(i); }
  const Cluster* getCluster(Index i) const { return &m_clusters.at(i); }

  Track* newTrack();
  Index numTracks() const { return static_cast<Index>(m_tracks.size()); }
  Track* getTrack(Index i) { return &m_tracks.at(i); }
  const Track* getTrack(Index i) const { return &m_tracks.at(i); }

  // deprecated accessors
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
