#ifndef __JD_EVENTLOOP_H__
#define __JD_EVENTLOOP_H__

#include <cstdint>
#include <memory>
#include <vector>

#include "progressbar.h"
#include "statistics.h"

namespace Analyzers {
class SingleAnalyzer;
}

namespace Processors {
class Processor;
}

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
            uint64_t startEvent = 0,
            uint64_t numEvents = 0);

  void addProcessor(std::unique_ptr<Processors::Processor> processor);
  void addAnalyzer(std::unique_ptr<Analyzers::SingleAnalyzer> analyzer);
  void run();

  std::unique_ptr<Storage::Event> readStartEvent();
  std::unique_ptr<Storage::Event> readEndEvent();

private:
  uint64_t numEvents() { return m_endEvent - m_startEvent + 1; }

  Storage::StorageIO* m_storage;
  uint64_t m_startEvent, m_endEvent;
  ProgressBar m_progress;
  EventStatistics m_stat;
  std::vector<std::unique_ptr<Processors::Processor>> m_processors;
  std::vector<std::unique_ptr<Analyzers::SingleAnalyzer>> m_analyzers;
};

} // namespace Utils

#endif // __JD_EVENTLOOP_H__
