// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

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
