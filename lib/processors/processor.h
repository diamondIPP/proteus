/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08-24
 */

#ifndef PT_PROCESSOR_H
#define PT_PROCESSOR_H

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
  virtual void process(Storage::Event& event) const = 0;
};

} // namespace Processors

#endif // PT_PROCESSOR_HO
