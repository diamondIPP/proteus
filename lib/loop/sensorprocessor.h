// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \author Moritz Kiehn <msmk@cern.ch>
 * \date 2019-09-03
 */

#pragma once

#include <string>

namespace proteus {

class SensorEvent;

/** Interface for algorithms to process and modify sensor events. */
class SensorProcessor {
public:
  virtual ~SensorProcessor() = default;
  virtual std::string name() const = 0;
  virtual void execute(SensorEvent&) const = 0;
};

} // namespace proteus
