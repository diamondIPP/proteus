/**
 * \author  Moritz Kiehn <msmk@cern.ch>
 * \date    2017-03
 */

#ifndef PT_READER_H
#define PT_READER_H

#include <cstdint>
#include <memory>
#include <string>

namespace Storage {
class Event;
}

namespace Io {

/** Event reader interface. */
class EventReader {
public:
  virtual ~EventReader();
  virtual std::string name() const = 0;
  /** Return the (minimum) number of available events.
   *
   * \returns UINT64_MAX if the number of events is unknown.
   *
   * Calling `readNext` the given number of times must succeed. Additional
   * calls could still succeed.
   */
  virtual uint64_t numEvents() const = 0;
  /** Skip the next n events.
   *
   * If the call would seek beyond the range of available events it should
   * not throw and error. Instead, the next `readNext` call should fail.
   */
  virtual void skip(uint64_t n) = 0;
  /** Read the next event from the underlying device into the given object.
   *
   * \returns true if an event was read
   * \returns false if no event was read because no more events are available
   *
   * The implementation is responsible for ensuring consistent events and
   * clearing previous contents. Errors must be handled by throwing
   * an appropriate exception.
   */
  virtual bool readNext(Storage::Event& event) = 0;
};

/** Open an event file with automatic determination of the file type. */
std::shared_ptr<EventReader> openRead(const std::string& path);

} // namespace Io

#endif // PT_READER_H
