#ifndef PT_EVENT_H
#define PT_EVENT_H

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "storage/plane.h"
#include "storage/track.h"

namespace Storage {

class Event {
public:
  Event(Index numPlanes);

  void clear();
  void setId(uint64_t id) { m_id = id; }
  void setFrameNumber(uint64_t frameNumber) { m_frameNumber = frameNumber; }
  void setTimeStamp(uint64_t timeStamp) { m_timeStamp = timeStamp; }
  void setTriggerOffset(unsigned int offset) { m_triggerOffset = offset; }
  void setTriggerInfo(unsigned int info) { m_triggerInfo = info; }
  void setTriggerPhase(unsigned int phase) { m_triggerPhase = phase; }
  void setInvalid(bool value) { m_invalid = value; }

  uint64_t id() const { return m_id; }
  uint64_t frameNumber() const { return m_frameNumber; }
  uint64_t timeStamp() const { return m_timeStamp; }
  unsigned int triggerOffset() const { return m_triggerOffset; }
  unsigned int triggerInfo() const { return m_triggerInfo; }
  unsigned int triggerPhase() const { return m_triggerPhase; }
  bool invalid() const { return m_invalid; }

  Index numPlanes() const { return static_cast<Index>(m_planes.size()); }
  Plane* getPlane(Index i) { return &m_planes.at(i); }
  const Plane* getPlane(Index i) const { return &m_planes.at(i); }

  /** Add track to the event and fix the cluster to track association. */
  void addTrackAndFreezeClusters(Track&& track);
  void addTrack(std::unique_ptr<Track> track);
  Index numTracks() const { return static_cast<Index>(m_tracks.size()); }
  Track* getTrack(Index i) { return m_tracks.at(i).get(); }
  const Track* getTrack(Index i) const { return m_tracks.at(i).get(); }

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

private:
  uint64_t m_id;
  uint64_t m_frameNumber;
  uint64_t m_timeStamp;
  unsigned int m_triggerOffset;
  unsigned int m_triggerInfo; // Dammit Andrej!
  unsigned int m_triggerPhase;
  bool m_invalid;

  std::vector<Plane> m_planes;
  std::vector<std::unique_ptr<Track>> m_tracks;
};

} // namespace Storage

#endif // PT_EVENT_H
