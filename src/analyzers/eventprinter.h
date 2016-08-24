#ifndef __JD_EVENTPRINTER_H__
#define __JD_EVENTPRINTER_H__

#include <memory>

#include "analyzer.h"

namespace Analyzers {

/** Print detailed information for each event. */
class EventPrinter : public Analyzer {
public:
  static std::unique_ptr<EventPrinter> make();

  std::string name() const;
  void analyze(uint64_t eventId, const Storage::Event& event);
  void finalize();

private:
  EventPrinter() = default;
};

} // namespace Analyzers

#endif // __JD_EVENTPRINTER_H__
