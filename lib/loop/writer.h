/**
 * \author  Moritz Kiehn <msmk@cern.ch>
 * \date    2017-03
 */

#pragma once

#include <string>

namespace proteus {

class Event;

/** Event writer interface. */
class Writer {
public:
  virtual ~Writer() = default;
  virtual std::string name() const = 0;
  /** Add the event to the underlying device.
   *
   * The reference to the event is only valid for the duration of the call.
   * Errors must be handled by throwing an appropriate exception.
   */
  virtual void append(const Event& event) = 0;
};

} // namespace proteus
