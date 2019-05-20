/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08-24
 */

#pragma once

#include <string>

namespace proteus {

class Event;

/** Interface for algorithms to process and modify events. */
class Processor {
public:
  virtual ~Processor() = default;
  virtual std::string name() const = 0;
  virtual void execute(Event&) const = 0;
};

} // namespace proteus
