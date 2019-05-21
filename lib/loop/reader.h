// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \author  Moritz Kiehn <msmk@cern.ch>
 * \date    2017-03
 */

#pragma once

#include <cstdint>
#include <string>

namespace proteus {

class Event;

/** Event reader interface. */
class Reader {
public:
  virtual ~Reader() = default;
  virtual std::string name() const = 0;
  /** Return the (minimum) number of available events.
   *
   * \returns UINT64_MAX if the number of events is unknown.
   *
   * Calling `read` the given number of times must succeed. Additional
   * calls could still succeed.
   */
  virtual uint64_t numEvents() const = 0;
  /** Return the number of sensors per event. */
  virtual size_t numSensors() const = 0;
  /** Skip the next n events.
   *
   * If the call would seek beyond the range of available events it should
   * not throw and error. Instead, the next `readNext` call should fail.
   */
  virtual void skip(uint64_t n) = 0;
  /** Read the next event from the underlying device into the given object.
   *
   * \param[out] event Output event with at least `numSensors()` sensor events.
   * \returns true if an event was read
   * \returns false if no event was read because no more events are available
   *
   * The Reader implementation is responsible for ensuring consistent events and
   * clearing previous contents. Errors must be handled by throwing an
   * appropriate exception.
   */
  virtual bool read(Event& event) = 0;
};

} // namespace proteus
