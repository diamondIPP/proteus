#ifndef PT_EVENTPRINTER_H
#define PT_EVENTPRINTER_H

#include "loop/analyzer.h"
#include "utils/logger.h"

namespace proteus {

/** Print detailed information for each event. */
class EventPrinter : public Analyzer {
public:
  EventPrinter();

  std::string name() const;
  void execute(const Event& event);

private:
  Logger m_logger;
};

} // namespace proteus

#endif // PT_EVENTPRINTER_H
