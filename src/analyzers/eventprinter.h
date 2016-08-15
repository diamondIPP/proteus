#ifndef __JD_EVENTPRINTER_H__
#define __JD_EVENTPRINTER_H__

#include <memory>

#include "singleanalyzer.h"

namespace Storage { class Event; }

namespace Analyzers {

/** Print detailed information for each event. */
class EventPrinter : public SingleAnalyzer {
public:
  static std::unique_ptr<EventPrinter> make();

  void processEvent(const Storage::Event* event);
  void postProcessing();

private:
  EventPrinter();

  uint64_t _events;
};

} // namespace Analyzers

#endif // __JD_EVENTPRINTER_H__
