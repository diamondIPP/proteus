#ifndef __JU_EVENTLOOP_H__
#define __JU_EVENTLOOP_H__

#include <cstdint>
#include <memory>
#include <vector>

#include "analyzers/analyzer.h"
#include "processors/processor.h"

namespace Analyzers {
class SingleAnalyzer;
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
  ~EventLoop();

  void addProcessor(std::shared_ptr<Processors::Processor> processor);
  void addAnalyzer(std::shared_ptr<Analyzers::Analyzer> analyzer);
  /** For backward compatibility: add old-style analyzer. */
  void addAnalyzer(std::shared_ptr<Analyzers::SingleAnalyzer> analyzer);
  void run();

  std::unique_ptr<Storage::Event> readStartEvent();
  std::unique_ptr<Storage::Event> readEndEvent();

private:
  uint64_t numEvents() { return m_endEvent - m_startEvent + 1; }

  Storage::StorageIO* m_storage;
  uint64_t m_startEvent, m_endEvent;
  std::vector<std::shared_ptr<Processors::Processor>> m_processors;
  std::vector<std::shared_ptr<Analyzers::Analyzer>> m_analyzers;
};

} // namespace Utils

#endif // __JU_EVENTLOOP_H__
