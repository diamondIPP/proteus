#ifndef PT_TIMEPIX3_H
#define PT_TIMEPIX3_H

#include <cstdint>
#include <string>
#include <vector>
#include <fstream>

#include <TDirectory.h>
#include <TFile.h>
#include <TTree.h>

#include "io/reader.h"
#include "io/writer.h"
#include "utils/definitions.h"

namespace Io {

/** Read events from a Timepix3 raw data file. */
class Timepix3Reader : public EventReader {
public:
  Timepix3Reader(const std::string& path);
  ~Timepix3Reader();

  std::string name() const;
  uint64_t numEvents() const { return UINT64_MAX; };

  void skip(uint64_t n);
  bool read(Storage::Event& event);

private:
    std::ifstream m_file;
    long long int m_syncTime;
    long long int m_prevTime;
    bool m_clearedHeader;
};

} // namespace Io

#endif // PT_TIMEPIX3_H
