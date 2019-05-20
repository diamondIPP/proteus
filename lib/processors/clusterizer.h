/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2016-10
 */

#pragma once

#include "loop/processor.h"

namespace proteus {

class Sensor;

/** Cluster hits and average the position with equal weights for all hits.
 *
 * The fastest hit time is used as the cluster time.
 */
class BinaryClusterizer : public Processor {
public:
  BinaryClusterizer(const Sensor& sensor) : m_sensor(sensor) {}

  std::string name() const;
  void execute(Event& event) const;

private:
  const Sensor& m_sensor;
};

/** Cluster hits and calculate position by weighting each hit with its value.
 *
 * The fastest hit time is used as the cluster time.
 */
class ValueWeightedClusterizer : public Processor {
public:
  ValueWeightedClusterizer(const Sensor& sensor) : m_sensor(sensor) {}

  std::string name() const;
  void execute(Event& event) const;

private:
  const Sensor& m_sensor;
};

/** Cluster hits and take position and timing only from the fastest hit. */
class FastestHitClusterizer : public Processor {
public:
  FastestHitClusterizer(const Sensor& sensor) : m_sensor(sensor) {}

  std::string name() const;
  void execute(Event& event) const;

private:
  const Sensor& m_sensor;
};

} // namespace proteus
