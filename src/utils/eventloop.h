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
#include "processors/processor.h"

namespace Storage {
class Event;
class StorageIO;
}

namespace Utils {

/** A generic event loop.
 *
 * Implements only the looping logic but no actual event
 * processing. Specific processing logic must be provided by
 * implementing processors and analyzers and adding them via
 * `addProcessor(...)` and `addAnalyzer(...)`. These will be called
 * for each event in the order in which they were added.
 */
class EventLoop {
public:
  EventLoop(Storage::StorageIO* storage,
            uint64_t start = 0,
            uint64_t events = UINT64_MAX);
  ~EventLoop();

  void enableProgressBar();
  void setOutput(Storage::StorageIO* output);

  void addProcessor(std::shared_ptr<Processors::Processor> processor);
  void addAnalyzer(std::shared_ptr<Analyzers::Analyzer> analyzer);
  void run();

  std::unique_ptr<Storage::Event> readStartEvent();
  std::unique_ptr<Storage::Event> readEndEvent();

private:
  uint64_t numEvents() { return m_numEvents; }

  Storage::StorageIO* m_input;
  Storage::StorageIO* m_output;
  uint64_t m_startEvent;
  uint64_t m_numEvents;
  bool m_showProgress;
  std::vector<std::shared_ptr<Processors::Processor>> m_processors;
  std::vector<std::shared_ptr<Analyzers::Analyzer>> m_analyzers;
};

} // namespace Utils

#endif // PT_EVENTLOOP_H
