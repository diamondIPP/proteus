#ifndef PT_EVENTPRINTER_H
#define PT_EVENTPRINTER_H

#include "loop/analyzer.h"

namespace Analyzers {

/** Print detailed information for each event. */
class EventPrinter : public Loop::Analyzer {
public:
  std::string name() const;
  void execute(const Storage::Event& event);
};

} // namespace Analyzers

#endif // PT_EVENTPRINTER_H
