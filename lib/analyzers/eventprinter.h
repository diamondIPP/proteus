#ifndef PT_EVENTPRINTER_H
#define PT_EVENTPRINTER_H

#include "loop/analyzer.h"

namespace Analyzers {

/** Print detailed information for each event. */
class EventPrinter : public Loop::Analyzer {
public:
  std::string name() const;
  void analyze(const Storage::Event& event);
  void finalize();
};

} // namespace Analyzers

#endif // PT_EVENTPRINTER_H
