/**
 * \file
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2016-08
 */

#ifndef PT_EVENTLOOP_H
#define PT_EVENTLOOP_H

#include <cstdint>
#include <memory>
#include <vector>

#include "analyzers/analyzer.h"
#include "io/io.h"
#include "processors/processor.h"

namespace Utils {

/** A generic event loop.
 *
 * Implements only the loop logic but not the actual event processing. Specific
 * processing logic must be provided by implementing processors and analyzers
 * and adding them via `addProcessor(...)` and `addAnalyzer(...)`. These will
 * be called for each event in the order in which they were added.
 *
 * The event loop gets its events from a single `EventReader` and can output
 * data to an arbitrary number of `EventWriter`s.
 */
class EventLoop {
public:
  EventLoop(Io::EventReader* reader,
            uint64_t start = 0,
            uint64_t events = UINT64_MAX);
  ~EventLoop();

  void enableProgressBar();

  void addProcessor(std::shared_ptr<Processors::Processor> processor);
  void addAnalyzer(std::shared_ptr<Analyzers::Analyzer> analyzer);
  void addWriter(std::shared_ptr<Io::EventWriter> writer);
  void run();

private:
  Io::EventReader* m_reader;
  std::vector<std::shared_ptr<Processors::Processor>> m_processors;
  std::vector<std::shared_ptr<Analyzers::Analyzer>> m_analyzers;
  std::vector<std::shared_ptr<Io::EventWriter>> m_writers;
  uint64_t m_start, m_events;
  bool m_showProgress;
};

} // namespace Utils

#endif // PT_EVENTLOOP_H
