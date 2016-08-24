/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \created 2016-08-24
 */

#ifndef __JU_ANALYZER_H__
#define __JU_ANALYZER_H__

#include <cstdint>
#include <string>

namespace Storage {
class Event;
}

namespace Analyzers {

class Analyzer {
public:
  virtual ~Analyzer();
  virtual std::string name() const = 0;
  virtual void analyze(uint64_t eventId, const Storage::Event& event) = 0;
  virtual void finalize() = 0;
};

} // namespace Analyzers

#endif // __JU_ANALYZER_H__
