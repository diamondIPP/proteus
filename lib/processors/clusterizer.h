// Copyright (c) 2014-2019 The Proteus authors
// SPDX-License-Identifier: MIT
/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#pragma once

#include "loop/sensorprocessor.h"

namespace proteus {

class Sensor;

/** Cluster hits and average the position with equal weights for all hits.
 *
 * The fastest hit time is used as the cluster time.
 */
class BinaryClusterizer : public SensorProcessor {
public:
  BinaryClusterizer(const Sensor& sensor) : m_sensor(sensor) {}

  std::string name() const;
  void execute(SensorEvent& sensorEvent) const;

private:
  const Sensor& m_sensor;
};

/** Cluster hits and calculate position by weighting each hit with its value.
 *
 * The fastest hit time is used as the cluster time.
 */
class ValueWeightedClusterizer : public SensorProcessor {
public:
  ValueWeightedClusterizer(const Sensor& sensor) : m_sensor(sensor) {}

  std::string name() const;
  void execute(SensorEvent& sensorEvent) const;

private:
  const Sensor& m_sensor;
};

/** Cluster hits and take position and timing only from the fastest hit. */
class FastestHitClusterizer : public SensorProcessor {
public:
  FastestHitClusterizer(const Sensor& sensor) : m_sensor(sensor) {}

  std::string name() const;
  void execute(SensorEvent& sensorEvent) const;

private:
  const Sensor& m_sensor;
};

} // namespace proteus
