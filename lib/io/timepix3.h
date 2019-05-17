#ifndef PT_TIMEPIX3_H
#define PT_TIMEPIX3_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "loop/reader.h"
#include "storage/sensorevent.h"
#include "utils/definitions.h"

namespace toml {
class Value;
}
namespace proteus {

/** Read events from a Timepix3 raw data file. */
class Timepix3Reader : public Reader {
public:
  /** Return a score of how likely the given path is an Timepix3 SPDR data file.
   */
  static int check(const std::string& path);
  /** Open the the file. */
  static std::shared_ptr<Timepix3Reader> open(const std::string& path,
                                              const toml::Value& /* unused */);

  Timepix3Reader(const std::string& path);
  ~Timepix3Reader(){};

  std::string name() const;
  uint64_t numEvents() const { return UINT64_MAX; };
  size_t numSensors() const { return 1; };

  void skip(uint64_t n);
  bool read(Event& event);

private:
  /** Returns one decoded sensorEvent for the current detector
   *
   * \returns SensorEvent for the current detector
   */
  bool getSensorEvent(SensorEvent& sensorEvent);

  /** File stream for the binary data file */
  std::ifstream m_file;

  long long int m_syncTime;
  long long int m_prevTime;
  bool m_clearedHeader;

  size_t m_eventNumber;
  double m_nextEventTimestamp;
};

} // namespace proteus

#endif // PT_TIMEPIX3_H
