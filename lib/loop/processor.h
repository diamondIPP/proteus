/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08-24
 */

#ifndef PT_PROCESSOR_H
#define PT_PROCESSOR_H

#include <string>

namespace Storage {
class Event;
}
namespace Loop {

/** Interface for algorithms to process and modify events. */
class Processor {
public:
  virtual ~Processor() = default;
  virtual std::string name() const = 0;
  virtual void process(Storage::Event&) const = 0;
};

} // namespace Loop

#endif // PT_PROCESSOR_H
