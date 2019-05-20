/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08-24
 */

#pragma once

#include <string>

namespace proteus {

class Event;

/** Interface for algorithms to analyze events. */
class Analyzer {
public:
  virtual ~Analyzer() = default;
  virtual std::string name() const = 0;
  virtual void execute(const Event&) = 0;
  /** The finalize method is optional. */
  virtual void finalize() {}
};

} // namespace proteus
