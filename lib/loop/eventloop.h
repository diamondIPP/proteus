// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace proteus {

class Analyzer;
class Processor;
class Reader;
class SensorProcessor;
class Writer;

/** A generic event processing loop.
 *
 * Implements only the loop logic but not the actual event processing. Specific
 * processing logic must be provided by implementing processors and analyzers
 * and adding them via `addSensorProcessor(...)`, `addProcessor(...)` and
 * `addAnalyzer(...)`. Per-sensor processors are executed first, followed by
 * the global processors, and finally by the analyzers. Within each group,
 * algorithms are executed in the order in which they are added.
 *
 * The event loop gets its events from a single `Reader` and can output
 * data to an arbitrary number of `Writer`s.
 */
class EventLoop {
public:
  EventLoop(std::shared_ptr<Reader> reader,
            size_t sensors,
            uint64_t start = 0,
            uint64_t events = UINT64_MAX,
            bool showProgress = false);
  ~EventLoop();

  void addSensorProcessor(size_t sensorId,
                          std::shared_ptr<SensorProcessor> sensorProcessor);
  void addProcessor(std::shared_ptr<Processor> processor);
  void addAnalyzer(std::shared_ptr<Analyzer> analyzer);
  void addWriter(std::shared_ptr<Writer> writer);
  void run();

private:
  std::shared_ptr<Reader> m_reader;
  std::map<size_t, std::vector<std::shared_ptr<SensorProcessor>>>
      m_sensorProcessors;
  std::vector<std::shared_ptr<Processor>> m_processors;
  std::vector<std::shared_ptr<Analyzer>> m_analyzers;
  std::vector<std::shared_ptr<Writer>> m_writers;
  uint64_t m_start, m_events;
  size_t m_sensors;
  bool m_showProgress;
};

} // namespace proteus
