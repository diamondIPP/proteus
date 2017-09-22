#ifndef PT_TIMEPIX3_H
#define PT_TIMEPIX3_H

#include <cstdint>
#include <string>
#include <vector>

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

};

} // namespace Io

#endif // PT_TIMEPIX3_H
