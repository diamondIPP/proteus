#ifndef EVENT_H
#define EVENT_H

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "storage/plane.h"
#include "storage/track.h"

namespace Processors {
class TrackFinder;
class TrackMaker;
}

namespace Storage {
class Event {
public:
  Event(Index numPlanes);

  void clear();
  void setId(uint64_t id) { m_id = id; }
  void setTimeStamp(uint64_t timeStamp) { m_timeStamp = timeStamp; }
  void setFrameNumber(uint64_t frameNumber) { m_frameNumber = frameNumber; }
  void setTriggerOffset(unsigned int offset) { m_triggerOffset = offset; }
  void setTriggerInfo(unsigned int info) { m_triggerInfo = info; }
  void setTriggerPhase(unsigned int phase) { m_triggerPhase = phase; }
  void setInvalid(bool value) { m_invalid = value; }

  uint64_t id() const { return m_id; }
  uint64_t timeStamp() const { return m_timeStamp; }
  uint64_t frameNumber() const { return m_frameNumber; }
  unsigned int triggerOffset() const { return m_triggerOffset; }
  unsigned int triggerInfo() const { return m_triggerInfo; }
  unsigned int triggerPhase() const { return m_triggerPhase; }
  bool invalid() const { return m_invalid; }

  Index numPlanes() const { return static_cast<Index>(m_planes.size()); }
  Plane* getPlane(Index i) { return &m_planes.at(i); }
  const Plane* getPlane(Index i) const { return &m_planes.at(i); }

  Track* newTrack();
  Index numTracks() const { return static_cast<Index>(m_tracks.size()); }
  Track* getTrack(Index i) { return &m_tracks.at(i); }
  const Track* getTrack(Index i) const { return &m_tracks.at(i); }

  // deprecated accessors
  unsigned int getNumHits() const;
  unsigned int getNumClusters() const;
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
  Track* addTrack(const Track& track);

  uint64_t m_id;
  uint64_t m_timeStamp;
  uint64_t m_frameNumber;
  unsigned int m_triggerOffset;
  unsigned int m_triggerInfo; // Dammit Andrej!
  unsigned int m_triggerPhase;
  bool m_invalid;

  std::vector<Plane> m_planes;
  std::vector<Track> m_tracks;

  friend class Processors::TrackFinder;
  friend class Processors::TrackMaker;
};

} // namespace Storage

#endif // EVENT_H
