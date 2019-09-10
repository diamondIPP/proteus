// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-02
 */

#pragma once

#include "loop/sensorprocessor.h"

namespace proteus {

class Sensor;

/** Assign region ids to hits using the sensor region information. */
class ApplyRegions : public SensorProcessor {
public:
  ApplyRegions(const Sensor& sensor);

  std::string name() const;
  void execute(SensorEvent& sensorEvent) const;

private:
  const Sensor& m_sensor;
};

} // namespace proteus
