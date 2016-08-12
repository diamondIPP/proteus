#ifndef __JD_EVENTPRINTER_H__
#define __JD_EVENTPRINTER_H__

#include "singleanalyzer.h"

namespace Storage { class Event; }

namespace Analyzers {

/** Print detailed information for each event. */
class EventPrinter : public SingleAnalyzer {
public:
  EventPrinter();

  void processEvent(const Storage::Event* event);
  void postProcessing();

private:
  uint64_t _events;
};

} // namespace Analyzers

#endif // __JD_EVENTPRINTER_H__
