#ifndef PT_EVENT_H
#define PT_EVENT_H

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include "storage/sensorevent.h"
#include "storage/track.h"

namespace Storage {

/** An event containing all global and local information for one trigger. */
class Event {
public:
  Event(size_t sensors);

  void clear(uint64_t frame, uint64_t timestamp);
  void setTrigger(int32_t info, int32_t offset, int32_t phase);
  void setInvalid(bool value) { m_invalid = value; }

  uint64_t frame() const { return m_frame; }
  uint64_t timestamp() const { return m_timestamp; }
  int32_t triggerInfo() const { return m_triggerInfo; }
  int32_t triggerOffset() const { return m_triggerOffset; }
  int32_t triggerPhase() const { return m_triggerPhase; }
  bool invalid() const { return m_invalid; }

  Index numSensorEvents() const { return static_cast<Index>(m_sensors.size()); }
  SensorEvent& getSensorEvent(Index i) { return m_sensors.at(i); }
  const SensorEvent& getSensorEvent(Index i) const { return m_sensors.at(i); }

  /** Add track to the event and fix the cluster to track association. */
  void addTrack(std::unique_ptr<Track> track);
  Index numTracks() const { return static_cast<Index>(m_tracks.size()); }
  Track& getTrack(Index i) { return *m_tracks.at(i).get(); }
  const Track& getTrack(Index i) const { return *m_tracks.at(i).get(); }

  size_t getNumHits() const;
  size_t getNumClusters() const;

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  uint64_t m_frame;
  uint64_t m_timestamp;
  int32_t m_triggerInfo; // Dammit Andrej!
  int32_t m_triggerOffset;
  int32_t m_triggerPhase;
  bool m_invalid;

  std::vector<SensorEvent> m_sensors;
  std::vector<std::unique_ptr<Track>> m_tracks;
};

} // namespace Storage

#endif // PT_EVENT_H
