/**
 * \author  Moritz Kiehn <msmk@cern.ch>
 * \date    2017-10
 */

#pragma once

#include <functional>
#include <map>
#include <memory>

#include "loop/reader.h"
#include "utils/definitions.h"

namespace eudaq {
class Event;
class FileReader;
} // namespace eudaq
namespace toml {
class Value;
}
namespace proteus {

/** Read Eudaq2 native raw files. */
class Eudaq2Reader : public Reader {
public:
  static int check(const std::string& path);
  static std::shared_ptr<Eudaq2Reader> open(const std::string& path,
                                            const toml::Value& cfg);

  Eudaq2Reader(const std::string& path);
  ~Eudaq2Reader();

  std::string name() const override final;
  uint64_t numEvents() const override final;
  size_t numSensors() const override final;

  void skip(uint64_t n) override final;
  bool read(Event& event) override final;

private:
  std::unique_ptr<::eudaq::FileReader,
                  std::function<void(::eudaq::FileReader*)>>
      m_reader;
  std::shared_ptr<const ::eudaq::Event> m_event;
  std::map<unsigned, Index> m_mapIdIndex;
};

} // namespace proteus
