#pragma once

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
