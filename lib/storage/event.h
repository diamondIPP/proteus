#ifndef PT_EVENT_H
#define PT_EVENT_H

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "storage/sensorevent.h"
#include "storage/track.h"

namespace proteus {

/** An event containing all global and local information for one trigger.
 *
 * The number of sensors per event is fixed at construction time.
 */
class Event {
public:
  explicit Event(size_t sensors);

  /** Clear the event without changing the number of sensors. */
  void clear(uint64_t frame = UINT64_MAX, uint64_t timestamp = UINT64_MAX);
  /** Set the data, i.e. hits and clusters, for one sensor.
   *
   * Reconstructed local track data is not copied.
   */
  void setSensorData(Index isensor, SensorEvent&& sensorEvent);
  /** Set the data, i.e. hits and clusters, for multiple sensors.
   *
   * This copies the data for all sensors in the input event and places them
   * into the sensors in this event starting at `first`. Reconstructed global
   * and local track data is not copied.
   */
  void setSensorData(Index first, Event&& event);

  uint64_t frame() const { return m_frame; }
  uint64_t timestamp() const { return m_timestamp; }

  Index numSensorEvents() const { return static_cast<Index>(m_sensors.size()); }
  SensorEvent& getSensorEvent(Index i) { return m_sensors.at(i); }
  const SensorEvent& getSensorEvent(Index i) const { return m_sensors.at(i); }

  /** Add track to the event and fix the cluster to track association. */
  void addTrack(const Track& track);
  Index numTracks() const { return static_cast<Index>(m_tracks.size()); }
  Track& getTrack(Index i) { return m_tracks.at(i); }
  const Track& getTrack(Index i) const { return m_tracks.at(i); }

  size_t getNumHits() const;
  size_t getNumClusters() const;

  void print(std::ostream& os, const std::string& prefix = std::string()) const;

private:
  uint64_t m_frame;
  uint64_t m_timestamp;

  std::vector<SensorEvent> m_sensors;
  std::vector<Track> m_tracks;
};

} // namespace proteus

#endif // PT_EVENT_H
