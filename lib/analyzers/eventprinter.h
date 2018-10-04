#ifndef PT_EVENTPRINTER_H
#define PT_EVENTPRINTER_H

#include "loop/analyzer.h"
#include "utils/logger.h"

namespace Analyzers {

/** Print detailed information for each event. */
class EventPrinter : public Loop::Analyzer {
public:
  EventPrinter();

  std::string name() const;
  void execute(const Storage::Event& event);

private:
  Utils::Logger m_logger;
};

} // namespace Analyzers

#endif // PT_EVENTPRINTER_H
