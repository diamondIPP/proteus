// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT

#pragma once

#include "loop/sensorprocessor.h"

namespace proteus {

class Sensor;

/** Transform Cartesian digital cluster coordinates into the local coordinates.
 *
 * Assumes the two digital coordinates are defined in a Cartesian coordinate
 * system, i.e. with orthogonal axes, with the scaling defined by the sensor
 * pitch.
 */
class ApplyLocalTransformCartesian : public SensorProcessor {
public:
  ApplyLocalTransformCartesian(const Sensor& device);

  std::string name() const;
  void execute(SensorEvent& sensorEvent) const;

private:
  const Sensor& m_sensor;
};

} // namespace proteus
