// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \author  Moritz Kiehn <msmk@cern.ch>
 * \date    2017-09
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "loop/reader.h"

namespace proteus {

/** Event merger that combines data from multiple readers.
 *
 * Assumes that the input streams are synchronized, i.e. the i-th event in each
 * data stream all belong to the same trigger or timestamp. The sensor events
 * from each reader are concatenated according to the order of the input
 * readers and to the order of the sensor events within each reader. They are
 * renumbered accordingly.
 *
 * Only sensor data, i.e. hits and clusters, are merged. Reconstructed data
 * is dropped.
 */
class EventMerger : public Reader {
public:
  EventMerger(std::vector<std::shared_ptr<Reader>> readers);

  std::string name() const override final;
  uint64_t numEvents() const override final { return m_events; }
  size_t numSensors() const override final { return m_sensors; }
  void skip(uint64_t n) override final;
  bool read(Event& event) override final;

private:
  std::vector<std::shared_ptr<Reader>> m_readers;
  uint64_t m_events;
  size_t m_sensors;
};

} // namespace proteus
