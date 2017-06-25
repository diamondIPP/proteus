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
  Event(size_t sensors);

  void clear();
  void setFrameNumber(uint64_t frameNumber) { m_frameNumber = frameNumber; }
  void setTimestamp(uint64_t timestamp) { m_timestamp = timestamp; }
  void setTriggerInfo(int32_t info) { m_triggerInfo = info; }
  void setTriggerOffset(int32_t offset) { m_triggerOffset = offset; }
  void setTriggerPhase(int32_t phase) { m_triggerPhase = phase; }
  void setInvalid(bool value) { m_invalid = value; }

  uint64_t frameNumber() const { return m_frameNumber; }
  uint64_t timestamp() const { return m_timestamp; }
  int32_t triggerInfo() const { return m_triggerInfo; }
  int32_t triggerOffset() const { return m_triggerOffset; }
  int32_t triggerPhase() const { return m_triggerPhase; }
  bool invalid() const { return m_invalid; }

  Index numPlanes() const { return static_cast<Index>(m_planes.size()); }
  Plane* getPlane(Index i) { return &m_planes.at(i); }
  const Plane* getPlane(Index i) const { return &m_planes.at(i); }

  /** Add track to the event and fix the cluster to track association. */
  void addTrack(std::unique_ptr<Track> track);
  Index numTracks() const { return static_cast<Index>(m_tracks.size()); }
  Track* getTrack(Index i) { return m_tracks.at(i).get(); }
  const Track* getTrack(Index i) const { return m_tracks.at(i).get(); }

  // deprecated accessors
  unsigned int getNumHits() const;
  unsigned int getNumClusters() const;

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  uint64_t m_frameNumber;
  uint64_t m_timestamp;
  int32_t m_triggerInfo; // Dammit Andrej!
  int32_t m_triggerOffset;
  int32_t m_triggerPhase;
  bool m_invalid;

  std::vector<Plane> m_planes;
  std::vector<std::unique_ptr<Track>> m_tracks;
};

} // namespace Storage

#endif // PT_EVENT_H
