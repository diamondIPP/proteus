/**
 * \author  Moritz Kiehn <msmk@cern.ch>
 * \date    2017-10
 */

#ifndef PT_EUDAQ_H
#define PT_EUDAQ_H

#include <map>
#include <memory>

#include "io/reader.h"
#include "utils/definitions.h"

namespace eudaq {
class FileReader;
} // namespace eudaq

namespace Io {

/** Read Eudaq native raw files. */
class EudaqReader : public EventReader {
public:
  static int check(const std::string& path);
  static std::shared_ptr<EudaqReader> open(const std::string& path,
                                           const toml::Value& cfg);

  EudaqReader(const std::string& path);
  ~EudaqReader();

  std::string name() const override final;
  uint64_t numEvents() const override final;
  size_t numSensors() const override final;

  void skip(uint64_t n) override final;
  bool read(Storage::Event& event) override final;

private:
  std::unique_ptr<eudaq::FileReader> m_reader;
  std::map<unsigned, Index> m_mapIdIndex;
  bool m_thatsIt;
};

} // namespace Io

#endif // PT_EUDAQ_H
