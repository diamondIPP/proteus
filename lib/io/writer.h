/**
 * \author  Moritz Kiehn <msmk@cern.ch>
 * \date    2017-03
 */

#ifndef PT_WRITER_H
#define PT_WRITER_H

#include <string>

namespace Storage {
class Event;
}

namespace Io {

/** Event writer interface. */
class EventWriter {
public:
  virtual ~EventWriter() = default;
  virtual std::string name() const = 0;
  /** Add the event to the underlying device.
   *
   * The reference to the event is only valid for the duration of the call.
   * Errors must be handled by throwing an appropriate exception.
   */
  virtual void append(const Storage::Event& event) = 0;
};

} // namespace Io

#endif // PT_WRITER_H
