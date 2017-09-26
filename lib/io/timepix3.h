#ifndef PT_TIMEPIX3_H
#define PT_TIMEPIX3_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include <TDirectory.h>
#include <TFile.h>
#include <TTree.h>

#include "io/reader.h"
#include "io/writer.h"
#include "storage/sensorevent.h"
#include "utils/definitions.h"

namespace Io {

/** Read events from a Timepix3 raw data file. */
class Timepix3Reader : public EventReader {
public:
  Timepix3Reader(const std::string& path);
  ~Timepix3Reader(){};

  std::string name() const;
  uint64_t numEvents() const { return UINT64_MAX; };
  size_t numSensors() const { return 1; };

  void skip(uint64_t n);
  bool read(Storage::Event& event);

private:
  /** Returns one decoded sensorEvent for the current detector
   *
   * \returns SensorEvent for the current detector
   */
  bool getSensorEvent(Storage::SensorEvent& sensorEvent);

  /** File stream for the binary data file */
  std::ifstream m_file;

  long long int m_syncTime;
  long long int m_prevTime;
  bool m_clearedHeader;

  size_t m_eventNumber;
  double m_nextEventTimestamp;
};

} // namespace Io

#endif // PT_TIMEPIX3_H
