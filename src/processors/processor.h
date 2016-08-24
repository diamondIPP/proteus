/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \created 2016-08-24
 */

#ifndef __JU_PROCESSOR_H__
#define __JU_PROCESSOR_H__

#include <cstdint>
#include <string>

namespace Storage {
class Event;
}

namespace Processors {

/** Interface for algorithms to process and modify events. */
class Processor {
public:
  virtual ~Processor();
  virtual std::string name() const = 0;
  virtual void processEvent(uint64_t eventId, Storage::Event& event) = 0;
  virtual void finalize() = 0;
};

} // namespace Processors

#endif // __JU_PROCESSOR_H__
