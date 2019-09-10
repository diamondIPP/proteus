// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#pragma once

#include "loop/sensorprocessor.h"
#include "utils/definitions.h"

namespace proteus {

/** Map FE-I4 digital address to correct CCPDv4 sensor pixel address. */
class CCPDv4HitMapper : public SensorProcessor {
public:
  CCPDv4HitMapper() = default;

  std::string name() const;
  void execute(SensorEvent& sensorEvent) const;
};

} // namespace proteus
